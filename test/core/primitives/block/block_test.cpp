/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/primitives/block/block.hpp"

#include <gtest/gtest.h>
#include "testutil/cbor.hpp"
#include "filecoin/common/hexutil.hpp"

/**
 * @given block header and its serialized representation from go
 * @when encode @and decode the block
 * @then decoded version matches the original @and encoded matches the go ones
 */
TEST(BlockTest, BlockHeaderCbor) {
  auto bls1 =
      "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
  auto bls2 =
      "020101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
  fc::primitives::block::BlockHeader block{
      fc::primitives::address::Address::makeFromId(1),
      boost::none,
      {
          fc::common::Buffer("F00D"_unhex),
          bls1,
          {},
      },
      {"010001020002"_cid},
      fc::primitives::BigInt(3),
      4,
      "010001020005"_cid,
      "010001020006"_cid,
      "010001020007"_cid,
      // TODO(turuslan): FIL-72 signature
      "01CAFE"_unhex,
      8,
      boost::none,
      9
  };
  expectEncodeAndReencode(
      block,
      "8d420001f68342f00d58600101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101018081d82a470001000102000242000304d82a4700010001020005d82a4700010001020006d82a47000100010200074301cafe08f609"_unhex);
  block.ticket = fc::primitives::ticket::Ticket{bls2};
  // TODO(turuslan): FIL-72 signature
  block.block_sig = "01DEAD"_unhex;
  expectEncodeAndReencode(
      block,
      "8d4200018158600201010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101018342f00d58600101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101018081d82a470001000102000242000304d82a4700010001020005d82a4700010001020006d82a47000100010200074301cafe084301dead09"_unhex);
}
