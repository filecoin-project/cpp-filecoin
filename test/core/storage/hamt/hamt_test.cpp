/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/hamt/hamt.hpp"

#include <gtest/gtest.h>
#include "codec/cbor/cbor_codec.hpp"
#include "common/which.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"

using fc::codec::cbor::encode;
using fc::common::which;
using fc::storage::hamt::Hamt;
using fc::storage::hamt::HamtError;
using fc::storage::hamt::Node;

class HamtTest : public ::testing::Test {
 public:
  std::shared_ptr<fc::storage::ipfs::IpfsDatastore> store_{
      std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  std::shared_ptr<Node> root_{std::make_shared<Node>(Node{{}, false})};
  Hamt hamt_{store_, root_, 8, false};
};

/** Hamt node CBOR encoding and decoding, correct CID */
TEST_F(HamtTest, NodeCbor) {
  Node n{{}, false};
  expectEncodeAndReencode(n, "824080"_unhex);

  n.items[17] = "010000020000"_cid;
  expectEncodeAndReencode(n, "824302000081a16130d82a4700010000020000"_unhex);

  n.items[17] =
      Node::Leaf{{"a", fc::storage::hamt::Value(encode("b").value())}};
  expectEncodeAndReencode(n, "824302000081a16131818241616162"_unhex);

  n.items[2] = Node::Leaf{{"b", fc::storage::hamt::Value(encode("a").value())}};
  expectEncodeAndReencode(
      n, "824302000482a16131818241626161a16131818241616162"_unhex);

  n.items[17] = Node::Ptr{};
  EXPECT_OUTCOME_ERROR(HamtError::kExpectedCID, encode(n));
}

/** Set-remove single element */
TEST_F(HamtTest, SetRemoveOne) {
  EXPECT_OUTCOME_ERROR(HamtError::kNotFound, hamt_.get("aai"));
  EXPECT_OUTCOME_ERROR(HamtError::kNotFound, hamt_.remove("aai"));

  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_EQ(root_->items.size(), 1);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("aai"));
  EXPECT_OUTCOME_ERROR(HamtError::kNotFound, hamt_.get("aai"));
  EXPECT_OUTCOME_ERROR(HamtError::kNotFound, hamt_.remove("aai"));
  EXPECT_EQ(root_->items.size(), 0);
}

/** Set-remove non-colliding elements */
TEST_F(HamtTest, SetRemoveNoCollision) {
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aaa", "02"_unhex));
  EXPECT_EQ(root_->items.size(), 2);
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("aaa"), "02"_unhex);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("aaa"));
  EXPECT_EQ(root_->items.size(), 1);
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_ERROR(HamtError::kNotFound, hamt_.get("aaa"));
}

/** Flush empty root */
TEST_F(HamtTest, FlushEmpty) {
  auto cidEmpty =
      "0171a0e4022018fe6acc61a3a36b0c373c4a3a8ea64b812bf2ca9b528050909c78d408558a0c"_cid;

  EXPECT_OUTCOME_EQ(store_->contains(cidEmpty), false);

  EXPECT_OUTCOME_TRUE_1(hamt_.flush());
  EXPECT_OUTCOME_EQ(store_->contains(cidEmpty), true);
}

/** Visit all key value pairs */
TEST_F(HamtTest, Visitor) {
  auto n = 0;
  Hamt::Visitor visitor = ([&n](auto k, auto v) {
    ++n;
    return fc::outcome::success();
  });

  EXPECT_OUTCOME_TRUE_1(hamt_.visit(visitor));
  EXPECT_EQ(n, 0);

  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ade", "02"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("agd", "03"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("agm", "04"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.visit(visitor));
  EXPECT_EQ(n, 4);
}

/** Visits all key value pairs after flush */
TEST_F(HamtTest, VisitorFlush) {
  auto n = 0;
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ade", "02"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.flush());
  EXPECT_OUTCOME_TRUE_1(hamt_.visit([&n](auto k, auto v) {
    ++n;
    return fc::outcome::success();
  }));
  EXPECT_EQ(n, 2);
}

/** Iteration stops after callback returns error */
TEST_F(HamtTest, VisitorError) {
  auto n = 0;
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ade", "02"_unhex));
  EXPECT_OUTCOME_ERROR(HamtError::kExpectedCID,
                       hamt_.visit([&n](auto k, auto v) {
                         ++n;
                         EXPECT_EQ(k, "aai");
                         EXPECT_EQ(v, "01"_unhex);
                         return HamtError::kExpectedCID;
                       }));
  EXPECT_EQ(n, 1);
}

/**
 * @given an empty HAMT
 * @when place an element
 * @then the element is present
 */
TEST_F(HamtTest, Contains) {
  EXPECT_OUTCOME_EQ(hamt_.contains("not_found"), false);
  EXPECT_OUTCOME_TRUE_1(hamt_.set("element", "01"_unhex));
  EXPECT_OUTCOME_EQ(hamt_.contains("element"), true);
}
