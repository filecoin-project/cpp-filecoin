/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spdlog/fmt/fmt.h"

#include "storage/car/car.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/leveldb/leveldb.hpp"

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

  auto leveldb_res = fc::storage::LevelDB::create(storage_dir, options);
  if (!leveldb_res) {
    fmt::print(stderr, "Cannot open or create leveldb store at {}", storage_dir);
    return __LINE__;
  }

  auto ipld =
      std::make_shared<fc::storage::ipfs::LeveldbDatastore>(leveldb_res.value());

  auto load_res = fc::storage::car::loadCar(*ipld, car_file);
  if (!load_res) {
    fmt::print(stderr, "Load snapshot failed, {}", load_res.error().message());
    return __LINE__;
  }

  // TODO do something with roots and index tipsets out there

  return 0;
}
