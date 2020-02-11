/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/amt/amt.hpp"

#include <gtest/gtest.h>
#include "testutil/cbor.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"

using fc::codec::cbor::encode;
using fc::common::which;
using fc::storage::amt::AmtError;
using fc::storage::amt::Amt;
using fc::storage::amt::Node;
using fc::storage::amt::Root;
using fc::storage::amt::Value;
using fc::storage::ipfs::InMemoryDatastore;

class AmtTest : public ::testing::Test {
 public:
  auto getRoot() {
    return store->getCbor<Root>(amt.flush().value()).value();
  }

  std::shared_ptr<InMemoryDatastore> store{std::make_shared<InMemoryDatastore>()};
  Amt amt{store};
};

/** Amt node CBOR encoding and decoding */
TEST_F(AmtTest, NodeCbor) {
  Root r;
  r.height = 1;
  r.count = 2;
  expectEncodeAndReencode(r, "83010283408080"_unhex);

  Node n;
  expectEncodeAndReencode(n, "83408080"_unhex);

  n.has_bits = true;
  expectEncodeAndReencode(n, "8341008080"_unhex);

  n.items = Node::Values{{2, Value{"01"_unhex}}};
  expectEncodeAndReencode(n, "834104808101"_unhex);

  n.items = Node::Links{{3, "010000020000"_cid}};
  expectEncodeAndReencode(n, "83410881d82a470001000002000080"_unhex);

  n.items = Node::Links{{3, Node::Ptr{}}};
  EXPECT_OUTCOME_ERROR(AmtError::EXPECTED_CID, encode(n));
}

TEST_F(AmtTest, SetRemoveRootLeaf) {
  auto key = 3llu;
  auto value = Value{"07"_unhex};

  EXPECT_OUTCOME_ERROR(AmtError::NOT_FOUND, amt.get(key));
  EXPECT_OUTCOME_ERROR(AmtError::NOT_FOUND, amt.remove(key));
  EXPECT_OUTCOME_EQ(amt.count(), 0);
  EXPECT_FALSE(getRoot().node.has_bits);

  EXPECT_OUTCOME_TRUE_1(amt.set(key, value));
  EXPECT_OUTCOME_EQ(amt.get(key), value);
  EXPECT_OUTCOME_EQ(amt.count(), 1);
  EXPECT_TRUE(getRoot().node.has_bits);

  EXPECT_OUTCOME_TRUE_1(amt.remove(key));
  EXPECT_OUTCOME_ERROR(AmtError::NOT_FOUND, amt.get(key));
  EXPECT_OUTCOME_EQ(amt.count(), 0);
  EXPECT_TRUE(getRoot().node.has_bits);
}

TEST_F(AmtTest, SetRemoveCollapseZero) {
  auto key = 64;

  EXPECT_OUTCOME_TRUE_1(amt.set(1, "06"_unhex));
  EXPECT_TRUE(which<Node::Values>(getRoot().node.items));

  EXPECT_OUTCOME_TRUE_1(amt.set(key, "07"_unhex));
  EXPECT_FALSE(which<Node::Values>(getRoot().node.items));

  EXPECT_OUTCOME_TRUE_1(amt.remove(key));
  EXPECT_TRUE(which<Node::Values>(getRoot().node.items));
}

TEST_F(AmtTest, Flush) {
  auto key = 9llu;
  auto value = Value{"07"_unhex};

  EXPECT_OUTCOME_TRUE_1(amt.set(key, value));
  EXPECT_OUTCOME_TRUE(cid, amt.flush());

  amt = {store, cid};
  EXPECT_OUTCOME_EQ(amt.get(key), value);
}

TEST_F(AmtTest, Visit) {
  std::vector<std::pair<int64_t, Value>> items{
    {3, Value{"06"_unhex}},
    {64, Value{"07"_unhex}}
  };
  for (auto &[key, value] : items) {
    EXPECT_OUTCOME_TRUE_1(amt.set(key, value));
  }

  auto i = 0;
  EXPECT_OUTCOME_TRUE_1(amt.visit([&](uint64_t key, const Value &value) {
    EXPECT_EQ(key, items[i].first);
    EXPECT_EQ(value, items[i].second);
    ++i;
    return fc::outcome::success();
  }));
  EXPECT_EQ(i, items.size());

  EXPECT_OUTCOME_ERROR(AmtError::INDEX_TOO_BIG, amt.visit([](uint64_t, const Value &) {
    return AmtError::INDEX_TOO_BIG;
  }));
}
