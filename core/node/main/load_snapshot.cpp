/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/filesystem.hpp>
#include "spdlog/fmt/fmt.h"

#include "builder.hpp"
#include "node/index_db_backend.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "vm/actor/cgo/actors.hpp"
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
      std::string storage_dir;
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
        fmt::print(stderr,
                   "Usage: load_snapshot car_file genesis_file storage_dir\n");
        return __LINE__;
      }

      c.car_file = argv[1];
      c.genesis_file = argv[2];
      c.storage_dir = argv[3];

      return 0;
    }

    int create_leveldb(bool must_be_empty) {
      leveldb::Options options;
      options.create_if_missing = must_be_empty;
      options.error_if_exists = must_be_empty;

      auto leveldb_res = storage::LevelDB::create(c.storage_dir, options);
      if (!leveldb_res) {
        fmt::print(
            stderr, "Cannot create leveldb store at {}\n", c.storage_dir);
        return __LINE__;
      }

      c.leveldb = std::move(leveldb_res.value());
      c.ipld = std::make_shared<storage::ipfs::LeveldbDatastore>(c.leveldb);
      return 0;
    }

    int load_genesis() {
      ASSERT2(c.leveldb);

      auto load_res = storage::car::loadCar(*c.leveldb, c.genesis_file);
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

      auto load_res = storage::car::loadCar(*c.leveldb, c.car_file);
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
      auto indexdb_res =
          sync::IndexDbBackend::create(c.storage_dir + node::kIndexDbFileName);
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

      fmt::print(stderr, "Indexing from height, {}\n", tipset->height());

      sync::TipsetInfo info;
      info.branch = sync::kGenesisBranch;

      size_t reported_percent = 0;
      size_t max_height = tipset->height();
      auto index_progress = [&](size_t height) {
        auto x = (max_height - height) * 100 / max_height;
        if (x > reported_percent) {
          reported_percent = x;
          fmt::print("index: {}%\n", reported_percent);
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

      fmt::print(stderr, "Indexing done\n");

      return 0;
    }

    int make_libp2p_key() {
      if (!node::persistLibp2pKey(c.storage_dir + node::kKeyFileName,
                                  boost::none)) {
        fmt::print(stderr, "Cannot write libp2p key\n");
        return __LINE__;
      }

      fmt::print(stderr, "Libp2p key created\n");
      return 0;
    }

    int interpret_finality_tipsets() {
      ASSERT2(c.leveldb);
      ASSERT2(c.ipld);
      ASSERT2(c.root_tipset);
      ASSERT2(c.genesis_cid.has_value());

      vm::actor::cgo::configMainnet();

      auto interpreter = std::make_shared<vm::interpreter::CachedInterpreter>(
          std::make_shared<vm::interpreter::InterpreterImpl>(
              std::make_shared<vm::runtime::TipsetRandomness>(c.ipld),
              vm::Circulating::make(c.ipld, c.genesis_cid.value()).value()),
          c.leveldb);

      auto tipset = c.root_tipset;
      unsigned tipsets_interpreted = 0;

      boost::optional<vm::interpreter::Result> expected_result;

      while (tipset->height() > 0) {
        //       common::Buffer key(tipset->key.hash());
        //       std::ignore = c.leveldb->remove(key);

        auto res = interpreter->interpret(c.ipld, tipset);
        if (!res) {
          fmt::print(stderr,
                     "Cannot interpret at height {}, {}\n",
                     tipset->height(),
                     res.error().message());
          return __LINE__;
        }

        if (expected_result.has_value()) {
          auto &result = res.value();
          if (result.state_root != expected_result->state_root) {
            fmt::print(stderr,
                       "State roots dont match at height {}\n",
                       tipset->height());
            return __LINE__;
          }
          if (result.message_receipts != expected_result->message_receipts) {
            fmt::print(stderr,
                       "Message receipts dont match at height {}\n",
                       tipset->height());
            return __LINE__;
          }
        }

        expected_result = vm::interpreter::Result{
            .state_root = tipset->getParentStateRoot(),
            .message_receipts = tipset->getParentMessageReceipts()};

        ++tipsets_interpreted;

        fmt::print("Interpreted height {}\n", tipset->height());

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

      // F.U.C.K
      common::Buffer key(gsl::span<const uint8_t>(
          reinterpret_cast<const uint8_t *>("finality"), 8));
      common::Buffer value(gsl::span<const uint8_t>(
          reinterpret_cast<const uint8_t *>(&finality_height),
          sizeof(finality_height)));
      auto res = c.leveldb->put(key, value);
      if (!res) {
        fmt::print(
            stderr, "Cannot persist finality. {}\n", res.error().message());
        return __LINE__;
      }

      fmt::print(stderr,
                 "Tipsets interpreted: {}, finality is set to height {}\n",
                 tipsets_interpreted,
                 finality_height);
      return 0;
    }

  }  // namespace

  int main(int argc, char *argv[]) {
    TRY_STEP(parse_args(argc, argv));

    bool must_be_empty = !boost::filesystem::exists(
        boost::filesystem::weakly_canonical(c.storage_dir));

    TRY_STEP(create_leveldb(must_be_empty));
    TRY_STEP(load_genesis());
    if (must_be_empty) {
      TRY_STEP(load_snapshot());
    }

    must_be_empty =
        !boost::filesystem::exists(boost::filesystem::weakly_canonical(
            c.storage_dir + node::kIndexDbFileName));

    TRY_STEP(create_indexdb(must_be_empty));
    TRY_STEP(make_root_tipset());

    if (must_be_empty) {
      TRY_STEP(index_snapshot());
      TRY_STEP(make_libp2p_key());
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
