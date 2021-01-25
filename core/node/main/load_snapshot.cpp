/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/filesystem.hpp>

#include "common/file.hpp"
#include "node/index_db_backend.hpp"
#include "node/main/builder.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/prefix.hpp"
#include "vm/actor/cgo/actors.hpp"
#include "vm/dvm/dvm.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/runtime/impl/tipset_randomness.hpp"

namespace fc {

#define TRY_STEP(Stmt) \
  if (int r = Stmt; r != 0) return r;

#define ASSERT2(Stmt)                                                         \
  if (!(Stmt)) {                                                              \
    fmt::print(stderr, "Condition failed, line {}: ({})\n", __LINE__, #Stmt); \
    return __LINE__;                                                          \
  }

  namespace {
    struct Ctx {
      std::string car_file;
      std::string genesis_file;
      boost::filesystem::path repo_path;
      std::shared_ptr<storage::LevelDB> leveldb;
      std::shared_ptr<storage::ipfs::IpfsDatastore> ipld;
      boost::optional<CID> genesis_cid;
      std::vector<CID> roots;
      primitives::tipset::TipsetCPtr root_tipset;
      std::shared_ptr<sync::IndexDbBackend> indexdb;
    };

    static Ctx c;

    int parse_args(int argc, char *argv[]) {
      if (argc != 4) {
        fmt::print(stderr, "Usage: load_snapshot car_file genesis_file repo\n");
        return __LINE__;
      }

      c.car_file = argv[1];
      c.genesis_file = argv[2];
      c.repo_path = argv[3];

      return 0;
    }

    outcome::result<std::vector<CID>> loadBigCar(
        storage::PersistentBufferMap &store, const std::string &file_name) {
      OUTCOME_TRY(file, common::mapFile(file_name));
      OUTCOME_TRY(reader, storage::car::CarReader::make(file.second));

      auto log = common::createLogger("load_car");

      log->info("reading file {} of size {}", file_name, reader.file.size());

      size_t reported_percent = 0;
      auto progress = [&]() {
        auto x = 100 * reader.position / reader.file.size();
        if (x > reported_percent) {
          reported_percent = x;
          log->info("{}%, {} objects stored", reported_percent, reader.objects);
        }
      };

      log->info("read header, {} roots", reader.roots.size());
      if (reader.roots.size() < 100) {
        for (const auto &r : reader.roots) {
          log->info("- {}", r);
        }
      }

      static constexpr size_t kBatchSize = 100000;
      size_t current_batch_size = 0;
      auto batch = store.batch();
      auto ipld{node::makeIpld(std::make_shared<storage::MapBatched>(*batch))};

      while (!reader.end()) {
        OUTCOME_TRY(item, reader.next());
        OUTCOME_TRY(ipld->set(item.first, Buffer{item.second}));

        ++current_batch_size;
        if (current_batch_size >= kBatchSize || reader.end()) {
          OUTCOME_TRY(batch->commit());
          batch->clear();
          current_batch_size = 0;
        }

        progress();
      }

      return std::move(reader.roots);
    }

    int create_leveldb(bool must_be_empty) {
      leveldb::Options options;
      options.create_if_missing = must_be_empty;
      options.error_if_exists = must_be_empty;

      auto leveldb_res = storage::LevelDB::create(
          (c.repo_path / node::kLeveldbPath).string(), options);
      if (!leveldb_res) {
        fmt::print(stderr, "Cannot create leveldb store at {}\n", c.repo_path);
        return __LINE__;
      }

      c.leveldb = std::move(leveldb_res.value());
      c.ipld = node::makeIpld(c.leveldb);
      return 0;
    }

    int load_genesis() {
      ASSERT2(c.leveldb);

      auto load_res = storage::car::loadCar(*c.ipld, c.genesis_file);
      if (!load_res) {
        fmt::print(
            stderr, "Load genesis failed, {}\n", load_res.error().message());
        return __LINE__;
      }

      if (load_res.value().size() != 1) {
        fmt::print(stderr,
                   "Genesis car file has {} != 1 roots\n",
                   load_res.value().size());
        return __LINE__;
      }

      c.genesis_cid = load_res.value()[0];

      return 0;
    }

    int load_snapshot() {
      ASSERT2(c.leveldb);

      auto load_res = loadBigCar(*c.leveldb, c.car_file);
      if (!load_res) {
        fmt::print(
            stderr, "Load snapshot failed, {}\n", load_res.error().message());
        return __LINE__;
      }

      c.roots = std::move(load_res.value());

      fmt::print(stderr, "{} roots loaded\n", c.roots.size());

      return 0;
    }

    int create_indexdb(bool must_be_empty) {
      auto indexdb_res = sync::IndexDbBackend::create(
          (c.repo_path / node::kIndexDbFileName).string());
      if (!indexdb_res) {
        fmt::print(stderr,
                   "Cannot create index db, {}\n",
                   indexdb_res.error().message());
        return __LINE__;
      }

      c.indexdb = std::move(indexdb_res.value());
      auto branches = c.indexdb->initDb();
      if (!branches) {
        fmt::print(
            stderr, "Cannot init index db, {}\n", branches.error().message());
        return __LINE__;
      }

      if (must_be_empty) {
        if (!branches.value().empty()) {
          fmt::print(stderr, "Index db must be empty\n");
          return __LINE__;
        }
        return 0;
      }

      auto it = branches.value().find(sync::kGenesisBranch);
      auto idx_res = c.indexdb->get(it->second->top, true);
      if (!idx_res) {
        fmt::print(stderr,
                   "Cannot get index of top tipset, {}\n",
                   idx_res.error().message());
        return __LINE__;
      }
      auto info_res = sync::IndexDbBackend::decode(std::move(idx_res.value()));
      if (!info_res) {
        fmt::print(stderr,
                   "Cannot get top tipset info, {}\n",
                   info_res.error().message());
        return __LINE__;
      }
      c.roots = info_res.value()->key.cids();
      fmt::print(
          stderr, "Found roots of height {}\n", info_res.value()->height);
      return 0;
    }

    int make_root_tipset() {
      ASSERT2(c.ipld);
      ASSERT2(!c.roots.empty());

      primitives::tipset::TipsetCreator creator;

      using primitives::block::BlockHeader;

      for (auto &cid : c.roots) {
        auto header_res = c.ipld->getCbor<BlockHeader>(cid);
        if (!header_res) {
          fmt::print(stderr,
                     "Root is not a block header, cid={}, {}\n",
                     cid.toString().value(),
                     header_res.error().message());
          return __LINE__;
        }
        auto &header = header_res.value();

        if (auto r = creator.canExpandTipset(header); !r) {
          fmt::print(stderr,
                     "Cannot expand highest tipset with header, cid={}, {}\n",
                     cid.toString().value(),
                     header_res.error().message());
          return __LINE__;
        }

        if (auto r = creator.expandTipset(cid, header); !r) {
          fmt::print(stderr,
                     "Cannot expand highest tipset with header, cid={}, {}\n",
                     cid.toString().value(),
                     header_res.error().message());
          return __LINE__;
        }
      }

      auto tipset = creator.getTipset(true);

      if (!c.ipld->contains(tipset->getParentStateRoot())) {
        fmt::print(stderr, "Snapshot doesnt contain state root\n");
        return __LINE__;
      }

      c.root_tipset = std::move(tipset);

      return 0;
    }

    int index_snapshot() {
      ASSERT2(c.indexdb);
      ASSERT2(c.root_tipset);
      ASSERT2(c.ipld);

      auto tx = c.indexdb->beginTx();

      auto tipset = c.root_tipset;

      spdlog::info("Indexing from height, {}", tipset->height());

      sync::TipsetInfo info;
      info.branch = sync::kGenesisBranch;

      size_t reported_percent = 0;
      size_t max_height = tipset->height();
      auto index_progress = [&](size_t height) {
        auto x = (max_height - height) * 100 / max_height;
        if (x > reported_percent) {
          reported_percent = x;
          spdlog::info("index: {}%", reported_percent);
        }
      };

      for (;;) {
        info.key = tipset->key;
        info.height = tipset->height();
        if (info.height > 0) {
          auto parent_res = tipset->loadParent(*c.ipld);
          if (!parent_res) {
            fmt::print(stderr,
                       "Cannot load parent at height {}, {}\n",
                       tipset->height(),
                       parent_res.error().message());
            return __LINE__;
          }
          tipset = std::move(parent_res.value());
          info.parent_hash = tipset->key.hash();
        } else {
          info.parent_hash.clear();
        }
        auto index_res = c.indexdb->store(info, boost::none);
        if (!index_res) {
          fmt::print(stderr,
                     "Cannot index tipset at height {}, {}\n",
                     info.height,
                     index_res.error().message());
          return __LINE__;
        }
        if (info.height == 0) {
          break;
        }
        index_progress(info.height);
      }

      tx.commit();

      spdlog::info("Indexing done");

      return 0;
    }

    int interpret_finality_tipsets() {
      ASSERT2(c.leveldb);
      ASSERT2(c.ipld);
      ASSERT2(c.root_tipset);
      ASSERT2(c.genesis_cid.has_value());

      vm::actor::cgo::configMainnet();

      if (dvm::logger) {
        dvm::logging = true;
        dvm::logger->flush_on(spdlog::level::info);
      }

      auto interpreter = std::make_shared<vm::interpreter::CachedInterpreter>(
          std::make_shared<vm::interpreter::InterpreterImpl>(
              std::make_shared<vm::runtime::TipsetRandomness>(c.ipld),
              vm::Circulating::make(c.ipld, c.genesis_cid.value()).value()),
          std::make_shared<storage::MapPrefix>(node::kCachedInterpreterPrefix,
                                               c.leveldb));

      auto tipset = c.root_tipset;
      unsigned tipsets_interpreted = 0;

      boost::optional<vm::interpreter::Result> expected_result;

      while (tipset->height() > 0) {
        spdlog::info("Interpreting height {}", tipset->height());

        auto res = interpreter->interpret(c.ipld, tipset);
        if (!res) {
          fmt::print(stderr,
                     "Cannot interpret at height {}, {}\n",
                     tipset->height(),
                     res.error().message());
          interpreter->removeCached(tipset);
          return __LINE__;
        }

        if (expected_result.has_value()) {
          auto &result = res.value();
          bool unexpected = false;
          if (result.state_root != expected_result->state_root) {
            fmt::print(stderr,
                       "State roots dont match at height {}\n",
                       tipset->height());
            unexpected = true;
          }
          if (result.message_receipts != expected_result->message_receipts) {
            fmt::print(stderr,
                       "Message receipts dont match at height {}\n",
                       tipset->height());
            unexpected = true;
          }
          if (unexpected) {
            interpreter->removeCached(tipset);
          }
        }

        expected_result = vm::interpreter::Result{
            .state_root = tipset->getParentStateRoot(),
            .message_receipts = tipset->getParentMessageReceipts()};

        ++tipsets_interpreted;

        auto parent_res = tipset->loadParent(*c.ipld);
        if (!parent_res) {
          fmt::print(stderr,
                     "Cannot load parent at height {}, {}\n",
                     tipset->height(),
                     parent_res.error().message());
          return __LINE__;
        }
        tipset = parent_res.value();

        if (!c.ipld->contains(tipset->getParentStateRoot())) {
          break;
        }
      }

      auto finality_height = tipset->height();

      spdlog::info("Tipsets interpreted: {}, finality is set to height {}",
                   tipsets_interpreted,
                   finality_height);
      return 0;
    }

  }  // namespace

  int main(int argc, char *argv[]) {
    TRY_STEP(parse_args(argc, argv));

    bool must_be_empty = !boost::filesystem::exists(
        boost::filesystem::weakly_canonical(c.repo_path));
    boost::filesystem::create_directories(c.repo_path);

    TRY_STEP(create_leveldb(must_be_empty));
    TRY_STEP(load_genesis());
    if (must_be_empty) {
      TRY_STEP(load_snapshot());
    }

    must_be_empty =
        !boost::filesystem::exists(boost::filesystem::weakly_canonical(
            c.repo_path / node::kIndexDbFileName));

    TRY_STEP(create_indexdb(must_be_empty));
    TRY_STEP(make_root_tipset());

    if (must_be_empty) {
      TRY_STEP(index_snapshot());
    }

    TRY_STEP(interpret_finality_tipsets());

    return 0;
  }
}  // namespace fc

int main(int argc, char *argv[]) {
  int line = fc::main(argc, argv);
  if (line != 0) {
    fmt::print(stderr, "Aborted at line {}\n", line);
    return 1;
  }
  return 0;
}
