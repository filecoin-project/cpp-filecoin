/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/leveldb_error.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::storage {
  namespace fs = boost::filesystem;

  struct LevelDB_Open : public test::BaseFS_Test {
    LevelDB_Open() : test::BaseFS_Test("fc_leveldb_open") {}
  };

  /**
   * @given options with disabled option `create_if_missing`
   * @when open database
   * @then database can not be opened (since there is no db already)
   */
  TEST_F(LevelDB_Open, OpenNonExistingDB) {
    leveldb::Options options;
    options.create_if_missing = false;  // intentionally

    const auto r = LevelDB::create(getPathString().string(), options);
    EXPECT_FALSE(r);
    EXPECT_EQ(r.error(), LevelDBError::kInvalidArgument);
  }

  /**
   * @given options with enable option `create_if_missing`
   * @when open database
   * @then database is opened
   */
  TEST_F(LevelDB_Open, OpenExistingDB) {
    leveldb::Options options;
    options.create_if_missing = true;  // intentionally

    EXPECT_OUTCOME_TRUE_2(db,
                          LevelDB::create(getPathString().string(), options));
    EXPECT_TRUE(db) << "db is nullptr";

    EXPECT_TRUE(fs::exists(getPathString()));
  }
}  // namespace fc::storage
