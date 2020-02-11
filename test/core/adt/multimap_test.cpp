/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "adt/multimap.hpp"
#include "common/outcome.hpp"
#include "storage/hamt/hamt.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "testutil/literals.hpp"

struct Fixture : public ::testing::Test {
  using Value = fc::adt::Multimap::Value;
  std::vector<Value> values_{
      Value{"06"_unhex}, Value{"07"_unhex}, Value{"08"_unhex}};
  std::shared_ptr<fc::storage::ipfs::IpfsDatastore> store_{
      std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  fc::adt::Multimap mmap_{store_};
  const std::string kKey = "mykey";

  fc::outcome::result<void> appendValues(fc::adt::Multimap &mmap) {
    for (const auto &v : values_) {
      if (auto res = mmap_.add(kKey, v); not res) {
        return res.error();
      }
    }
    return fc::outcome::success();
  }

  void checkValues(fc::adt::Multimap &mmap) {
    auto index = 0u;
    EXPECT_OUTCOME_TRUE_1(mmap_.visit(
        kKey, [this, &index](const fc::adt::Multimap::Value &value) {
          EXPECT_EQ(values_[index], value);
          ++index;
          return fc::outcome::success();
        }))
    EXPECT_EQ(index, values_.size());
  }
};

/**
 * @given an empty Array container
 * @when it is saved to IPLD storage
 * @then its root CID equals to expected
 */
TEST_F(Fixture, BasicEmpty) {
  auto res = mmap_.flush();
  ASSERT_TRUE(res);
  // empty CID is generated on golang side
  auto cidEmpty =
      "0171a0e4022018fe6acc61a3a36b0c373c4a3a8ea64b812bf2ca9b528050909c78d408558a0c"_cid;
  ASSERT_EQ(res.value(), cidEmpty);
}

/**
 * @given a Multimap
 * @when it is initialized with a set of values
 * @then that values can be accessed by the key in an expected order
 */
TEST_F(Fixture, OrderIsPreserved) {
  EXPECT_OUTCOME_TRUE_1(appendValues(mmap_));
  checkValues(mmap_);
}

/// Multimap can be fully-functionally reconstructed from its root CID
TEST_F(Fixture, AccessByCid) {
  EXPECT_OUTCOME_TRUE_1(appendValues(mmap_));
  EXPECT_OUTCOME_TRUE(mmap_root, mmap_.flush());
  fc::adt::Multimap mmap{store_, mmap_root};
  checkValues(mmap);
}

/**
 * @given an initialized Multimap
 * @when removeAll is called for the known key
 * @then underlying HAMT also removes that key
 */
TEST_F(Fixture, UnderlyingHamtAmt) {
  EXPECT_OUTCOME_TRUE_1(appendValues(mmap_));
  EXPECT_OUTCOME_TRUE_1(mmap_.flush());
  checkValues(mmap_);

  EXPECT_OUTCOME_TRUE_1(mmap_.removeAll(kKey));
  EXPECT_OUTCOME_TRUE(hamt_root, mmap_.flush());

  fc::storage::hamt::Hamt hamt{store_, hamt_root};
  auto amt_root = hamt.getCbor<fc::CID>(kKey);
  ASSERT_FALSE(amt_root);
  ASSERT_EQ(fc::storage::hamt::HamtError::NOT_FOUND, amt_root.error());
}
