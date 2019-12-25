/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/storage/base_leveldb_test.hpp"

namespace test {

  BaseLevelDB_Test::BaseLevelDB_Test(const fs::path &path)
      : BaseFS_Test(path) {}

  void BaseLevelDB_Test::open() {
    leveldb::Options options;
    options.create_if_missing = true;

    auto r = LevelDB::create(getPathString(), options);
    if (!r) {
      throw std::invalid_argument(r.error().message());
    }

    db_ = std::move(r.value());
    ASSERT_TRUE(db_) << "BaseLevelDB_Test: db is nullptr";
  }

  void BaseLevelDB_Test::SetUp() {
    open();
  }

  void BaseLevelDB_Test::TearDown() {
    clear();
  }
}  // namespace test
