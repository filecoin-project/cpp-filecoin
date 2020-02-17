/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/hamt/hamt.hpp"

#include <gtest/gtest.h>
#include "codec/cbor/cbor.hpp"
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
  auto bit(size_t i) {
    return root_->items.find(i) != root_->items.end();
  }

  decltype(auto) minItem(const Node &node) {
    return node.items.begin()->second;
  }

  template <typename T>
  auto minItemIs(const Node &node) {
    return which<T>(minItem(node));
  }

  std::shared_ptr<fc::storage::ipfs::IpfsDatastore> store_{
      std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  std::shared_ptr<Node> root_{std::make_shared<Node>()};
  Hamt hamt_{store_, root_};
};

/** Hamt node CBOR encoding and decoding, correct CID */
TEST_F(HamtTest, NodeCbor) {
  Node n;
  expectEncodeAndReencode(n, "824080"_unhex);

  n.items[17] = "010000020000"_cid;
  expectEncodeAndReencode(n, "824302000081a16130d82a4700010000020000"_unhex);

  n.items[17] =
      Node::Leaf{{"a", fc::storage::hamt::Value(encode("b").value())}};
  expectEncodeAndReencode(n, "824302000081a16131818261616162"_unhex);

  n.items[2] = Node::Leaf{{"b", fc::storage::hamt::Value(encode("a").value())}};
  expectEncodeAndReencode(
      n, "824302000482a16131818261626161a16131818261616162"_unhex);

  n.items[17] = Node::Ptr{};
  EXPECT_OUTCOME_ERROR(HamtError::EXPECTED_CID, encode(n));
}

/** Set-remove single element */
TEST_F(HamtTest, SetRemoveOne) {
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.get("aai"));
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.remove("aai"));

  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_TRUE(bit(253));
  EXPECT_EQ(root_->items.size(), 1);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("aai"));
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.get("aai"));
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.remove("aai"));
  EXPECT_FALSE(bit(253));
  EXPECT_EQ(root_->items.size(), 0);
}

/** Set-remove non-colliding elements */
TEST_F(HamtTest, SetRemoveNoCollision) {
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aaa", "02"_unhex));
  EXPECT_TRUE(bit(253));
  EXPECT_TRUE(bit(190));
  EXPECT_EQ(root_->items.size(), 2);
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("aaa"), "02"_unhex);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("aaa"));
  EXPECT_TRUE(bit(253));
  EXPECT_FALSE(bit(190));
  EXPECT_EQ(root_->items.size(), 1);
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.get("aaa"));
}

/** Set-remove kLeafMax colliding elements, does not shard */
TEST_F(HamtTest, SetRemoveCollisionMax) {
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ade", "02"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("agd", "03"_unhex));
  EXPECT_TRUE(bit(253));
  EXPECT_EQ(root_->items.size(), 1);
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("ade"), "02"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("agd"), "03"_unhex);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("ade"));
  EXPECT_OUTCOME_TRUE_1(hamt_.remove("agd"));
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.get("ade"));
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.get("agd"));
}

/** Set-remove kLeafMax + 1 colliding elements, creates shard */
TEST_F(HamtTest, SetRemoveCollisionChild) {
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ade", "02"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("agd", "03"_unhex));
  EXPECT_TRUE(minItemIs<Node::Leaf>(*root_));

  EXPECT_OUTCOME_TRUE_1(hamt_.set("agm", "04"_unhex));
  EXPECT_TRUE(minItemIs<Node::Ptr>(*root_));
  EXPECT_EQ(boost::get<Node::Ptr>(minItem(*root_))->items.size(), 4);
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("ade"), "02"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("agd"), "03"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("agm"), "04"_unhex);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("agm"));
  // shard of leaves with key count <= kLeafMax collapses
  EXPECT_TRUE(minItemIs<Node::Leaf>(*root_));
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("ade"), "02"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("agd"), "03"_unhex);
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.get("agm"));
}

/** Set-remove kLeafMax + 1 double colliding elements, creates two nested shards
 */
TEST_F(HamtTest, SetRemoveDoubleCollisionChild) {
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ails", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aufx", "02"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("bmvm", "03"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("cnyh", "04"_unhex));
  EXPECT_TRUE(minItemIs<Node::Ptr>(*root_));
  auto &child = *boost::get<Node::Ptr>(minItem(*root_));
  EXPECT_TRUE(minItemIs<Node::Ptr>(child));

  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "05"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ade", "06"_unhex));
  EXPECT_EQ(child.items.size(), 3);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("ade"));
  EXPECT_TRUE(minItemIs<Node::Ptr>(child));
  EXPECT_EQ(child.items.size(), 2);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("cnyh"));
  // shard of leaves with key count > kLeafMax does not collapse
  EXPECT_TRUE(minItemIs<Node::Ptr>(*root_));
  // shard of leaf collapses
  EXPECT_TRUE(minItemIs<Node::Leaf>(child));
  EXPECT_EQ(child.items.size(), 2);
}

/** Flush empty root */
TEST_F(HamtTest, FlushEmpty) {
  auto cidEmpty =
      "0171a0e4022018fe6acc61a3a36b0c373c4a3a8ea64b812bf2ca9b528050909c78d408558a0c"_cid;

  EXPECT_OUTCOME_EQ(store_->contains(cidEmpty), false);

  EXPECT_OUTCOME_TRUE_1(hamt_.flush());
  EXPECT_OUTCOME_EQ(store_->contains(cidEmpty), true);
}

/** Flush node of leafs, intermediate state not stored */
TEST_F(HamtTest, FlushNoCollision) {
  auto cidWithLeaf =
      "0171a0e4022055e50395a85788650fc83874f45442e531f6289b0402d95ef3da8b01870c2629"_cid;

  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.remove("aai"));
  EXPECT_OUTCOME_TRUE_1(hamt_.flush());
  EXPECT_OUTCOME_EQ(store_->contains(cidWithLeaf), false);

  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.flush());
  EXPECT_OUTCOME_EQ(store_->contains(cidWithLeaf), true);
}

/** Flush node with shard, intermediate state not stored */
TEST_F(HamtTest, FlushCollisionChild) {
  auto cidShard =
      "0171a0e402209431b57360c7ee799ffbc1f6fa83dce5239eb48f7fef6cb167190fd15267daf0"_cid;

  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ade", "02"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("agd", "03"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("agm", "04"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.remove("agm"));
  EXPECT_OUTCOME_TRUE_1(hamt_.flush());
  EXPECT_OUTCOME_EQ(store_->contains(cidShard), false);

  EXPECT_OUTCOME_TRUE_1(hamt_.set("agm", "04"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.flush());
  EXPECT_OUTCOME_EQ(store_->contains(cidShard), true);
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
  EXPECT_OUTCOME_ERROR(HamtError::EXPECTED_CID,
                       hamt_.visit([&n](auto k, auto v) {
                         ++n;
                         EXPECT_EQ(k, "aai");
                         EXPECT_EQ(v, "01"_unhex);
                         return HamtError::EXPECTED_CID;
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

/**
 * @given empty HAMT
 * @when place empty value
 * @then value is present
 */
TEST_F(HamtTest, PlaceEmpty) {
  EXPECT_OUTCOME_TRUE_1(hamt_.setCbor<std::vector<int>>("key", {}));
  EXPECT_OUTCOME_TRUE_1(hamt_.flush());
  EXPECT_OUTCOME_EQ(hamt_.contains("key"), true);
}
