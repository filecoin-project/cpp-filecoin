/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <array>
#include <boost/filesystem.hpp>
#include <exception>

#include "common/hexutil.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/leveldb_error.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_leveldb_test.hpp"

namespace fc::storage {
  namespace fs = boost::filesystem;

  struct LevelDB_Integration_Test : public test::BaseLevelDB_Test {
    LevelDB_Integration_Test()
        : test::BaseLevelDB_Test("fc_leveldb_integration_test") {}

    Bytes key_{1, 3, 3, 7};
    Bytes value_{1, 2, 3};
  };

  /**
   * @given opened database, with {key}
   * @when read {key}
   * @then {value} is correct
   */
  TEST_F(LevelDB_Integration_Test, Put_Get) {
    EXPECT_OUTCOME_TRUE_1(db_->put(key_, BytesIn{value_}));
    EXPECT_TRUE(db_->contains(key_));
    EXPECT_OUTCOME_TRUE_2(val, db_->get(key_));
    EXPECT_EQ(val, value_);
  }

  /**
   * @given empty db
   * @when read {key}
   * @then get "not found"
   */
  TEST_F(LevelDB_Integration_Test, Get_NonExistent) {
    EXPECT_FALSE(db_->contains(key_));
    EXPECT_OUTCOME_TRUE_1(db_->remove(key_));
    auto r = db_->get(key_);
    EXPECT_FALSE(r);
    EXPECT_EQ(r.error().value(), (int)LevelDBError::kNotFound);
  }

  /**
   * @given database with [(i,i) for i in range(6)]
   * @when create batch and write KVs
   * @then data is written only after commit
   */
  TEST_F(LevelDB_Integration_Test, WriteBatch) {
    std::list<Bytes> keys{{0}, {1}, {2}, {3}, {4}, {5}};
    Bytes toBeRemoved = {3};
    std::list<Bytes> expected{{0}, {1}, {2}, {4}, {5}};

    auto batch = db_->batch();
    ASSERT_TRUE(batch);

    for (const auto &item : keys) {
      EXPECT_OUTCOME_TRUE_1(batch->put(item, BytesIn{item}));
      EXPECT_FALSE(db_->contains(item));
    }
    EXPECT_OUTCOME_TRUE_1(batch->remove(toBeRemoved));
    EXPECT_OUTCOME_TRUE_1(batch->commit());

    for (const auto &item : expected) {
      EXPECT_TRUE(db_->contains(item));
      EXPECT_OUTCOME_TRUE_2(val, db_->get(item));
      EXPECT_EQ(val, item);
    }

    EXPECT_FALSE(db_->contains(toBeRemoved));
  }

  /**
   * @given database with [(i,i) for i in range(100)]
   * @when iterate over kv pairs forward and backward
   * @then we iterate over all items
   */
  TEST_F(LevelDB_Integration_Test, Iterator) {
    const size_t size = 100;
    // 100 buffers of size 1 each; 0..99
    std::list<Bytes> keys;
    for (size_t i = 0; i < size; i++) {
      keys.emplace_back(1, i);
    }

    for (const auto &item : keys) {
      EXPECT_OUTCOME_TRUE_1(db_->put(item, BytesIn{item}));
    }

    std::array<size_t, size> counter{};

    logger->warn("forward iteration");
    auto it = db_->cursor();
    for (it->seekToFirst(); it->isValid(); it->next()) {
      auto k = it->key();
      auto v = it->value();
      EXPECT_EQ(k, v);

      logger->info(
          "key: {}, value: {}", common::hex_upper(k), common::hex_upper(v));

      EXPECT_GE(k[0], 0);
      EXPECT_LT(k[0], size);
      EXPECT_GT(k.size(), 0);

      counter[k[0]]++;
    }

    for (size_t i = 0; i < size; i++) {
      EXPECT_EQ(counter[i], 1);
    }

    logger->warn("backward iteration");
    size_t c = 0;
    uint8_t index = 0xf;
    Bytes seekTo{index};
    // seek to `index` element
    for (it->seek(seekTo); it->isValid(); it->prev()) {
      auto k = it->key();
      auto v = it->value();
      EXPECT_EQ(k, v);

      logger->info(
          "key: {}, value: {}", common::hex_upper(k), common::hex_upper(v));

      c++;
    }

    EXPECT_FALSE(it->isValid());
    EXPECT_EQ(c, index + 1);
  }
}  // namespace fc::storage
