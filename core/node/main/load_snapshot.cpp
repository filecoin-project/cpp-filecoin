/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spdlog/fmt/fmt.h"

#include "primitives/block/block.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/leveldb/leveldb.hpp"

namespace fc {
  int main(int argc, char *argv[]) {
    if (argc != 3) {
      fmt::print(stderr, "Usage: load_snapshot car_file storage_dir");
      return __LINE__;
    }

    std::string car_file = argv[1];
    std::string storage_dir = argv[2];

    leveldb::Options options;
    options.create_if_missing = true;
    options.error_if_exists = false;

    auto leveldb_res = storage::LevelDB::create(storage_dir, options);
    if (!leveldb_res) {
      fmt::print(
          stderr, "Cannot open or create leveldb store at {}", storage_dir);
      return __LINE__;
    }

    auto ipld =
        std::make_shared<storage::ipfs::LeveldbDatastore>(leveldb_res.value());

    auto load_res = storage::car::loadCar(*ipld, car_file);
    if (!load_res) {
      fmt::print(
          stderr, "Load snapshot failed, {}", load_res.error().message());
      return __LINE__;
    }

    fmt::print(stderr, "Snapshot loaded");

    auto roots = load_res.value();

    // find highest and lowest tipsets in the snapshot

    using primitives::block::BlockHeader;

    std::vector<BlockHeader> lowest_blocks;
    std::vector<BlockHeader> highest_blocks;
    uint64_t lowest_height = -1;
    uint64_t highest_height = 0;
    uint64_t lowest_height_with_parent_state_root = -1;

    size_t reported_percent = 0;
    size_t current_root = 0;
    size_t total_roots = roots.size();
    auto roots_progress = [&]() {
      auto x = current_root * 100 / total_roots;
      if (x > reported_percent) {
        reported_percent = x;
        fmt::print(
            "roots traverse: {}%, lowest_height={}, highest_height={},"
            " lowest_height_with_parent_state_root={}",
            reported_percent,
            lowest_height,
            highest_height,
            lowest_height_with_parent_state_root);
      }
    };

    for (const auto &cid : roots) {
      auto header_res = ipld->getCbor<BlockHeader>(cid);
      if (!header_res) {
        continue;
      }
      auto &header = header_res.value();

      if (header.height > 0
          && header.height < lowest_height_with_parent_state_root) {
        auto r = ipld->contains(header.parent_state_root);
        if (r.has_value() && r.value()) {
          lowest_height_with_parent_state_root = header.height;
        }
      }

      if (header.height > highest_height) {
        highest_height = header.height;
        highest_blocks.clear();
        highest_blocks.push_back(std::move(header));
      } else if (header.height < lowest_height) {
        lowest_height = header.height;
        lowest_blocks.clear();
        lowest_blocks.push_back(std::move(header));
      } else if (header.height == lowest_height) {
        lowest_blocks.push_back(std::move(header));
      } else if (header.height == highest_height) {
        highest_blocks.push_back(std::move(header));
      }

      ++current_root;
      roots_progress();
    }

    // TODO index tipsets
    // create tipset from highest blocks
    // index it down
    // write snapshot "trusted bottom"

    return 0;
  }
}  // namespace fc

int main(int argc, char *argv[]) {
  return fc::main(argc, argv);
}
