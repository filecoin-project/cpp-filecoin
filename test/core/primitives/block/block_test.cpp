/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/block/block.hpp"

#include <gtest/gtest.h>
#include "common/hexutil.hpp"
#include "testutil/cbor.hpp"
#include "testutil/crypto/sample_signatures.hpp"

/**
 * @given block header and its serialized representation from go
 * @when encode @and decode the block
 * @then decoded version matches the original @and encoded matches the go ones
 */
TEST(BlockTest, BlockHeaderCbor) {
  const auto kSampleBlsSignatureBytes2 =
      "020101010101010101010101010101010101010101010101010101010101010101010101"
      "010101010101010101010101010101010101010101010101010101010101010101010101"
      "010101010101010101010101010101010101010101010101"_blob96;

  fc::primitives::block::BlockHeader block{
      fc::primitives::address::Address::makeFromId(1),
      boost::none,
      {fc::common::Buffer{"F00D"_unhex}},
      {fc::primitives::block::BeaconEntry{
          4,
          "F00D"_unhex,
      }},
      {fc::primitives::sector::PoStProof{
          fc::primitives::sector::RegisteredProof::StackedDRG2KiBSeal,
          "F00D"_unhex,
      }},
      {"010001020002"_cid},
      fc::primitives::BigInt(3),
      4,
      "010001020005"_cid,
      "010001020006"_cid,
      "010001020007"_cid,
      boost::none,
      8,
      boost::none,
      9,
  };
  expectEncodeAndReencode(
      block,
      "8f420001f68142f00d81820442f00d81820342f00d81d82a470001000102000242000304d82a4700010001020005d82a4700010001020006d82a4700010001020007f608f609"_unhex);
  block.ticket = fc::primitives::ticket::Ticket{kSampleBlsSignatureBytes2};
  block.bls_aggregate = fc::crypto::signature::Signature{kSampleBlsSignature};
  block.block_sig = fc::crypto::bls::Signature{kSampleBlsSignatureBytes2};
  expectEncodeAndReencode(
      block,
      "8f4200018158600201010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101018142f00d81820442f00d81820342f00d81d82a470001000102000242000304d82a4700010001020005d82a4700010001020006d82a47000100010200075861020101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010858610202010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010109"_unhex);
}
