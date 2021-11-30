/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "markets/storage/mk_protocol.hpp"
#include "testutil/markets/protocol/protocol_named_cbor_fixture.hpp"

namespace fc::markets::storage {
  using vm::actor::builtin::types::market::ClientDealProposal;

  /**
   * Tests storage market deal protocol.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   */
  class MkProtocolTest : public ProtocolNamedCborTestFixture {
   public:
    Signature deal_signature{crypto::signature::Secp256k1Signature{}};

    DataRef piece{"manual", cid, cid, UnpaddedPieceSize(256), 42};

    Response response{
        StorageDealStatus::STORAGE_DEAL_UNKNOWN, "message", cid, cid};
  };

  /**
   * Compatible with go lotus encoding
   */
  TEST_F(MkProtocolTest, ProposalCborNamedDecodeFromGo) {
    ClientDealProposal client_deal_proposal{deal_proposal, deal_signature};
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
    SignedResponseV1_1_0 signed_response{response};
    sign(signed_response);

    const auto go_encoded = normalizeMap(
        "a268526573706f6e7365a465537461746500674d657373616765676d6573736167656850726f706f73616cd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6e5075626c6973684d657373616765d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a695369676e6174757265586102ac390db02d674db198c1c842a094f84ba4b4818fd9413f9e858a4fe3a98b7f82aea6279587aa43ac2ce4c958ba980b86103bed01ad6269a74263903778ef3c30e0c4c0c2d4d1860ebf54239b244f964242a49df1208867b7a582f5bad7d1f918"_unhex);
    expectEncodeAndReencode(signed_response, go_encoded);

    verify(signed_response);
  }

}  // namespace fc::markets::storage
