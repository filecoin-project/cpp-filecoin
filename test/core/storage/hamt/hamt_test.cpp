/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/hamt/hamt.hpp"

#include "codec/cbor/cbor.hpp"
#include "common/which.hpp"
#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"

using fc::codec::cbor::decode;
using fc::codec::cbor::encode;
using fc::common::which;
using fc::storage::hamt::CID;
using fc::storage::hamt::Hamt;
using fc::storage::hamt::HamtError;
using fc::storage::hamt::Node;

template<typename T>
void expectEncodeAndReencode(const T &value, const std::vector<uint8_t> &bytes) {
  EXPECT_OUTCOME_EQ(encode(value), bytes);
  EXPECT_OUTCOME_TRUE(decoded, decode<T>(bytes));
  EXPECT_OUTCOME_EQ(encode(decoded), bytes);
}

void expectCid(const Node &n, const std::vector<uint8_t> &expected) {
  EXPECT_OUTCOME_TRUE(cid, n.cid());
  EXPECT_EQ(libp2p::multi::ContentIdentifierCodec::encode(cid).value(), expected);
}

CID decodeCid(const std::vector<uint8_t> &bytes) {
  return libp2p::multi::ContentIdentifierCodec::decode(bytes).value();
}

class HamtTest : public ::testing::Test {
 public:
  std::shared_ptr<fc::storage::ipfs::IpfsDatastore> store_{std::make_shared<fc::storage::ipfs::InMemoryDatastore>()}; 
  std::shared_ptr<Node> root_{std::make_shared<Node>()};
  Hamt hamt_{store_, root_};
};

/** Hamt node CBOR encoding and decoding, correct CID */
TEST_F(HamtTest, NodeCbor) {
  Node n;
  expectEncodeAndReencode(n, "824080"_unhex);
  expectCid(n, "0171a0e4022018fe6acc61a3a36b0c373c4a3a8ea64b812bf2ca9b528050909c78d408558a0c"_unhex);

  n.bits |= 1 << 17;
  expectEncodeAndReencode(n, "824302000080"_unhex);
  expectCid(n, "0171a0e40220ccece38b1ed05d6ff6e7158aaaf0cec9ed99aa5e0cd453d6365de2439f05cd4b"_unhex);

  Node::Leaf leaf;
  leaf["a"] = fc::storage::hamt::Value(encode("b").value());
  n.items.push_back(leaf);
  expectEncodeAndReencode(n, "824302000081a16131818261616162"_unhex);
  expectCid(n, "0171a0e40220190d7c4481ea44aa30e79618e3299271031f9eed6b33912c494b88bb07288917"_unhex);

  n.items.push_back(decodeCid("010000020000"_unhex));
  expectEncodeAndReencode(n, "824302000082a16131818261616162a16130d82a4700010000020000"_unhex);
  expectCid(n, "0171a0e40220d0a4fe1e4666cda17148cc41812042a7341bd6fee0f36433a9a804f4fa6c0845"_unhex);

  n.items.push_back(Node::Ptr{});
  EXPECT_OUTCOME_ERROR(HamtError::EXPECTED_CID, encode(n));
}

/** Set-remove single element */
TEST_F(HamtTest, SetRemoveOne) {
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.get("aai"));
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.remove("aai"));

  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_TRUE(bit_test(root_->bits, 253));
  EXPECT_EQ(root_->items.size(), 1);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("aai"));
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.get("aai"));
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.remove("aai"));
  EXPECT_FALSE(bit_test(root_->bits, 253));
  EXPECT_EQ(root_->items.size(), 0);
}

/** Set-remove non-colliding elements */
TEST_F(HamtTest, SetRemoveNoCollision) {
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aaa", "02"_unhex));
  EXPECT_TRUE(bit_test(root_->bits, 253));
  EXPECT_TRUE(bit_test(root_->bits, 190));
  EXPECT_EQ(root_->items.size(), 2);
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("aaa"), "02"_unhex);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("aaa"));
  EXPECT_TRUE(bit_test(root_->bits, 253));
  EXPECT_FALSE(bit_test(root_->bits, 190));
  EXPECT_EQ(root_->items.size(), 1);
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.get("aaa"));
}

/** Set-remove kLeafMax colliding elements, does not shard */
TEST_F(HamtTest, SetRemoveCollisionMax) {
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ade", "02"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("agd", "03"_unhex));
  EXPECT_TRUE(bit_test(root_->bits, 253));
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
  EXPECT_TRUE(which<Node::Leaf>(root_->items[0]));

  EXPECT_OUTCOME_TRUE_1(hamt_.set("agm", "04"_unhex));
  EXPECT_TRUE(which<Node::Ptr>(root_->items[0]));
  EXPECT_EQ(boost::get<Node::Ptr>(root_->items[0])->items.size(), 4);
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("ade"), "02"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("agd"), "03"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("agm"), "04"_unhex);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("agm"));
  // shard of leaves with key count <= kLeafMax collapses
  EXPECT_TRUE(which<Node::Leaf>(root_->items[0]));
  EXPECT_OUTCOME_EQ(hamt_.get("aai"), "01"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("ade"), "02"_unhex);
  EXPECT_OUTCOME_EQ(hamt_.get("agd"), "03"_unhex);
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, hamt_.get("agm"));
}

/** Set-remove kLeafMax + 1 double colliding elements, creates two nested shards */
TEST_F(HamtTest, SetRemoveDoubleCollisionChild) {
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ails", "01"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("aufx", "02"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("bmvm", "03"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("cnyh", "04"_unhex));
  EXPECT_TRUE(which<Node::Ptr>(root_->items[0]));
  auto &child = *boost::get<Node::Ptr>(root_->items[0]);
  EXPECT_TRUE(which<Node::Ptr>(child.items[0]));

  EXPECT_OUTCOME_TRUE_1(hamt_.set("aai", "05"_unhex));
  EXPECT_OUTCOME_TRUE_1(hamt_.set("ade", "06"_unhex));
  EXPECT_EQ(child.items.size(), 3);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("ade"));
  EXPECT_TRUE(which<Node::Ptr>(child.items[0]));
  EXPECT_EQ(child.items.size(), 2);

  EXPECT_OUTCOME_TRUE_1(hamt_.remove("cnyh"));
  // shard of leaves with key count > kLeafMax does not collapse
  EXPECT_TRUE(which<Node::Ptr>(root_->items[0]));
  // shard of leaf collapses
  EXPECT_TRUE(which<Node::Leaf>(child.items[0]));
  EXPECT_EQ(child.items.size(), 2);
}

/** Flush empty root */
TEST_F(HamtTest, FlushEmpty) {
  auto cidEmpty = decodeCid("0171a0e4022018fe6acc61a3a36b0c373c4a3a8ea64b812bf2ca9b528050909c78d408558a0c"_unhex);

  EXPECT_OUTCOME_EQ(store_->contains(cidEmpty), false);

  EXPECT_OUTCOME_TRUE_1(hamt_.flush());
  EXPECT_OUTCOME_EQ(store_->contains(cidEmpty), true);
}

/** Flush node of leafs, intermediate state not stored */
TEST_F(HamtTest, FlushNoCollision) {
  auto cidWithLeaf = decodeCid("0171a0e4022055e50395a85788650fc83874f45442e531f6289b0402d95ef3da8b01870c2629"_unhex);

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
  auto cidShard = decodeCid("0171a0e402209431b57360c7ee799ffbc1f6fa83dce5239eb48f7fef6cb167190fd15267daf0"_unhex);

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
  EXPECT_OUTCOME_ERROR(HamtError::EXPECTED_CID, hamt_.visit([&n](auto k, auto v) {
    ++n;
    EXPECT_EQ(k, "aai");
    EXPECT_EQ(v, "01"_unhex);
    return HamtError::EXPECTED_CID;
  }));
  EXPECT_EQ(n, 1);
}
