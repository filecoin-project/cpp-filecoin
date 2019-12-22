/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/hamt/hamt.hpp"

#include "codec/cbor/cbor.hpp"
#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::codec::cbor::decode;
using fc::codec::cbor::encode;
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

TEST(Hamt, NodeCbor) {
  Node n;
  expectEncodeAndReencode(n, "824080"_unhex);
  expectCid(n, "0171a0e4022018fe6acc61a3a36b0c373c4a3a8ea64b812bf2ca9b528050909c78d408558a0c"_unhex);

  n.bits |= 1 << 17;
  expectEncodeAndReencode(n, "824302000080"_unhex);
  expectCid(n, "0171a0e40220ccece38b1ed05d6ff6e7158aaaf0cec9ed99aa5e0cd453d6365de2439f05cd4b"_unhex);

  Node::Leaf leaf;
  leaf["a"] = encode("b").value();
  n.items.push_back(leaf);
  expectEncodeAndReencode(n, "824302000081a16131818261616162"_unhex);
  expectCid(n, "0171a0e40220190d7c4481ea44aa30e79618e3299271031f9eed6b33912c494b88bb07288917"_unhex);

  n.items.push_back(libp2p::multi::ContentIdentifierCodec::decode("010000020000"_unhex).value());
  expectEncodeAndReencode(n, "824302000082a16131818261616162a16130d82a4700010000020000"_unhex);
  expectCid(n, "0171a0e40220d0a4fe1e4666cda17148cc41812042a7341bd6fee0f36433a9a804f4fa6c0845"_unhex);

  n.items.push_back(Node::Ptr{});
  EXPECT_OUTCOME_ERROR(HamtError::EXPECTED_CID, encode(n));
}
