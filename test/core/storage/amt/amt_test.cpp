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
using fc::storage::amt::Node;
using fc::storage::amt::Root;
using fc::storage::amt::Value;

class AmtTest : public ::testing::Test {
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
