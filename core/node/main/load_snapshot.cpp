/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spdlog/fmt/fmt.h"

#include "builder.hpp"
#include "node/index_db_backend.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/leveldb/leveldb.hpp"

namespace fc {
  int main(int argc, char *argv[]) {
    if (argc != 4) {
      fmt::print(stderr,
                 "Usage: load_snapshot car_file genesis_file storage_dir");
      return __LINE__;
    }

    std::string car_file = argv[1];
    std::string genesis_file = argv[2];
    std::string storage_dir = argv[3];

    leveldb::Options options;
    options.create_if_missing = true;
    options.error_if_exists = true;

    auto leveldb_res = storage::LevelDB::create(storage_dir, options);
    if (!leveldb_res) {
      fmt::print(stderr, "Cannot create leveldb store at {}", storage_dir);
      return __LINE__;
    }

    auto &leveldb = leveldb_res.value();

    auto load_res = storage::car::loadCar(*leveldb, genesis_file);
    if (!load_res) {
      fmt::print(stderr, "Load genesis failed, {}", load_res.error().message());
      return __LINE__;
    }

    if (load_res.value().size() != 1) {
      fmt::print(stderr,
                 "Genesis car file has {} != 1 roots",
                 load_res.value().size());
      return __LINE__;
    }

    load_res = storage::car::loadCar(*leveldb, car_file);
    if (!load_res) {
      fmt::print(
          stderr, "Load snapshot failed, {}", load_res.error().message());
      return __LINE__;
    }

    auto roots = load_res.value();

    fmt::print(stderr, "{} roots loaded", roots.size());

    // roots must be a tipset

    auto ipld = std::make_shared<storage::ipfs::LeveldbDatastore>(leveldb);

    primitives::tipset::TipsetCreator creator;

    using primitives::block::BlockHeader;

    for (auto &cid : roots) {
      auto header_res = ipld->getCbor<BlockHeader>(cid);
      if (!header_res) {
        fmt::print(stderr,
                   "Root is not a block header, cid={}, {}",
                   cid.toString().value(),
                   header_res.error().message());
        return __LINE__;
      }
      auto &header = header_res.value();

      if (auto r = creator.canExpandTipset(header); !r) {
        fmt::print(stderr,
                   "Cannot expand highest tipset with header, cid={}, {}",
                   cid.toString().value(),
                   header_res.error().message());
        return __LINE__;
      }

      if (auto r = creator.expandTipset(cid, header); !r) {
        fmt::print(stderr,
                   "Cannot expand highest tipset with header, cid={}, {}",
                   cid.toString().value(),
                   header_res.error().message());
        return __LINE__;
      }
    }

    auto tipset = creator.getTipset(true);

    if (!ipld->contains(tipset->getParentStateRoot())) {
      fmt::print(stderr, "Snapshot doesnt contain state root");
      return __LINE__;
    }

    // Index all what is in the snapshot

    auto indexdb_res =
        sync::IndexDbBackend::create(storage_dir + node::kIndexDbFileName);
    if (!indexdb_res) {
      fmt::print(
          stderr, "Cannot create index db, {}", load_res.error().message());
      return __LINE__;
    }

    auto &indexdb = indexdb_res.value();

    auto tx = indexdb->beginTx();

    fmt::print(stderr, "Indexing from height, {}", tipset->height());

    sync::TipsetInfo info;
    info.branch = sync::kGenesisBranch;

    size_t reported_percent = 0;
    size_t max_height = tipset->height();
    auto index_progress = [&](size_t height) {
      auto x = height * 100 / max_height;
      if (x > reported_percent) {
        reported_percent = x;
        fmt::print("index: {}%", reported_percent);
      }
    };

    for (;;) {
      info.key = tipset->key;
      info.height = tipset->height();
      if (info.height > 0) {
        auto parent_res = tipset->loadParent(*ipld);
        if (!parent_res) {
          fmt::print(stderr,
                     "Cannot load parent at height {}, {}",
                     tipset->height(),
                     parent_res.error().message());
          return __LINE__;
        }
        tipset = std::move(parent_res.value());
        info.parent_hash = tipset->key.hash();
      } else {
        info.parent_hash.clear();
      }
      auto index_res = indexdb->store(info, boost::none);
      if (!index_res) {
        fmt::print(stderr,
                   "Cannot index tipset at height {}, {}",
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

    fmt::print(stderr, "Indexing done");

    if (!node::persistLibp2pKey(storage_dir + node::kKeyFileName,
                                boost::none)) {
      fmt::print(stderr, "Cannot write libp2p key");
      return __LINE__;
    }

    fmt::print(stderr, "Done");

    return 0;
  }
}  // namespace fc

int main(int argc, char *argv[]) {
  return fc::main(argc, argv);
}
