/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "markets/storage/deal_protocol.hpp"
#include "testutil/cbor.hpp"

namespace fc::markets::storage {
  using crypto::signature::Signature;
  using primitives::address::Address;
  using primitives::address::decodeFromString;
  using primitives::piece::PaddedPieceSize;
  using primitives::piece::UnpaddedPieceSize;
  using vm::actor::builtin::types::market::ClientDealProposal;
  using vm::actor::builtin::types::market::DealProposal;

  /**
   * Tests storage market deal protocol.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   */
  class MkProtocolTest : public ::testing::Test {
   public:
    // address from go test constants
    Address address =
        decodeFromString("t2i4llai5x72clnz643iydyplvjmni74x4vyme7ny").value();
    // CID from go-generated string
    CID cid = CID::fromString("QmTTA2daxGqo5denp6SwLzzkLJm3fuisYEi9CoWsuHpzfb")
                  .value();
    Signature signature{crypto::signature::Secp256k1Signature{}};

    DealProposal deal_proposal{
        .piece_cid = cid,
        .piece_size = PaddedPieceSize{256},
        .verified = true,
        .client = address,
        .provider = address,
        .label = "label",
        .start_epoch = 101,
        .end_epoch = 2002,
        .storage_price_per_epoch = 22,
        .provider_collateral = 333,
        .client_collateral = 4444,
    };

    ClientDealProposal client_deal_proposal{deal_proposal, signature};

    DataRef piece{"manual", cid, cid, UnpaddedPieceSize(256), 42};

    Response response{StorageDealStatus::STORAGE_DEAL_UNKNOWN, "message", cid, cid};
  };

  /**
   * Compatible with go lotus encoding
   */
  TEST_F(MkProtocolTest, ProposalCborNamedDecodeFromGo) {
    ProposalV1_1_0 proposal{{client_deal_proposal, piece, true}};
    const auto go_encoded = normalizeMap(
        "a36c4465616c50726f706f73616c828bd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a190100f555024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc55024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc656c6162656c18651907d24200164300014d4300115c5842010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000655069656365a56c5472616e7366657254797065666d616e75616c64526f6f74d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a685069656365436964d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a69506965636553697a651901006c526177426c6f636b53697a65182a6d4661737452657472696576616cf5"_unhex);
    expectEncodeAndReencode(proposal, go_encoded);
  }

  TEST_F(MkProtocolTest, ResponseCborNamedDecodeFromGo) {
    ResponseV1_1_0 responsev1_1_0{response};
    const auto go_encoded = normalizeMap(
        "a465537461746500674d657373616765676d6573736167656850726f706f73616cd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6e5075626c6973684d657373616765d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a"_unhex);
    expectEncodeAndReencode(responsev1_1_0, go_encoded);
  }

  TEST_F(MkProtocolTest, SignedResponseCborNamedDecodeFromGo) {
    SignedResponseV1_1_0 signed_response{response, signature};
    const auto go_encoded = normalizeMap(
        "a268526573706f6e7365a465537461746500674d657373616765676d6573736167656850726f706f73616cd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6e5075626c6973684d657373616765d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a695369676e61747572655842010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"_unhex);
    expectEncodeAndReencode(signed_response, go_encoded);
  }

}  // namespace fc::markets::storage