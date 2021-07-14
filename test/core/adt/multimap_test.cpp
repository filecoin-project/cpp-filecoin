/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "adt/multimap.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::adt::Multimap;

struct MultimapTest : public ::testing::Test {
  using Value = int;
  using Array = fc::adt::Array<Value>;
  using Map = fc::adt::Map<Array>;

  std::vector<Value> values{1, 2, 3};
  std::shared_ptr<fc::storage::ipfs::IpfsDatastore> ipld{
      std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  Map mmap{ipld};
  fc::Bytes key{4, 5, 6};

  void appendValues() {
    for (auto value : values) {
      EXPECT_OUTCOME_TRUE_1(Multimap::append(mmap, key, value));
    }
  }

  void expectVisitValues() {
    auto index = 0u;
    EXPECT_OUTCOME_TRUE_1(Multimap::visit(mmap, key, {[&](auto value) {
                                            EXPECT_EQ(values[index], value);
                                            ++index;
                                            return fc::outcome::success();
                                          }}));
    EXPECT_EQ(index, values.size());
  }
};

/**
 * @given an empty Array container
 * @when it is saved to IPLD storage
 * @then its root CID equals to expected
 */
TEST_F(MultimapTest, BasicEmpty) {
  EXPECT_OUTCOME_TRUE_1(mmap.hamt.flush());
  // empty CID is generated on golang side
  EXPECT_EQ(
      mmap.hamt.cid(),
      "0171a0e4022018fe6acc61a3a36b0c373c4a3a8ea64b812bf2ca9b528050909c78d408558a0c"_cid);
}

/**
 * @given multimap with no items on key
 * @when visit key
 * @then no items visited
 */
TEST_F(MultimapTest, VisitEmpty) {
  values.clear();
  expectVisitValues();
}

/**
 * @given multimap with no items on key
 * @when append values and visit
 * @then items visited in order of insertion
 */
TEST_F(MultimapTest, AppendAndVisit) {
  appendValues();
  expectVisitValues();
}

/**
 * @given multimap with no items on key
 * @when append values, flush, reload from cid, and visit
 * @then items visited in order of insertion
 */
TEST_F(MultimapTest, ReloadFromCid) {
  appendValues();
  EXPECT_OUTCOME_TRUE_1(mmap.hamt.flush());
  mmap = {mmap.hamt.cid(), ipld};
  expectVisitValues();
}
