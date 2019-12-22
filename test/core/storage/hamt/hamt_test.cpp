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

TEST(Hamt, NodeCbor) {
  Node n;
  expectEncodeAndReencode(n, "824080"_unhex);

  n.bits |= 1 << 17;
  expectEncodeAndReencode(n, "824302000080"_unhex);

  Node::Leaf leaf;
  leaf["a"] = encode("b").value();
  n.items.push_back(leaf);
  expectEncodeAndReencode(n, "824302000081a16131818261616162"_unhex);

  n.items.push_back(libp2p::multi::ContentIdentifierCodec::decode("010000020000"_unhex).value());
  expectEncodeAndReencode(n, "824302000082a16131818261616162a16130d82a4700010000020000"_unhex);

  n.items.push_back(Node::Ptr{});
  EXPECT_OUTCOME_ERROR(HamtError::EXPECTED_CID, encode(n));
}
