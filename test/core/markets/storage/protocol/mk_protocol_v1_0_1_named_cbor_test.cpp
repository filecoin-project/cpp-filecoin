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
   * Tests storage market deal protocol V1.0.1.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   */
  class MkProtocolV1_0_1Test : public ProtocolNamedCborTestFixture {
   public:
    Signature deal_signature{crypto::signature::Secp256k1Signature{}};

    DataRef piece{"manual", cid, cid, UnpaddedPieceSize(256), 42};

    Response response{
        StorageDealStatus::STORAGE_DEAL_UNKNOWN, "message", cid, cid};
  };

  /**
   * Compatible with go lotus encoding
   */
  TEST_F(MkProtocolV1_0_1Test, ProposalCborNamedDecodeFromGo) {
    ClientDealProposal client_deal_proposal{deal_proposal, deal_signature};
    ProposalV1_0_1 proposal{{client_deal_proposal, piece, true}};
    const auto go_encoded =
        "83828bd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a190100f555024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc55024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc656c6162656c18651907d24200164300014d4300115c584201000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000084666d616e75616cd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459ad82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a190100f5"_unhex;
    expectEncodeAndReencode(proposal, go_encoded);
  }

  TEST_F(MkProtocolV1_0_1Test, ResponseCborNamedDecodeFromGo) {
    ResponseV1_0_1 responsev1_1_0{response};
    const auto go_encoded =
        "8400676d657373616765d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459ad82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a"_unhex;
    expectEncodeAndReencode(responsev1_1_0, go_encoded);
  }

  TEST_F(MkProtocolV1_0_1Test, SignedResponseCborNamedDecodeFromGo) {
    SignedResponseV1_0_1 signed_response{response};
    sign(signed_response);

    const auto go_encoded =
        "828400676d657373616765d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459ad82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a586102926d7a246dc353a080d021e5a6b25358610d7bb0a6ef267c6589caec9ff02b4e890f0204fc7c1bd1728a7edf750a7aa7024c42f70ffea72b32a618d32a2d2f05beeba01cbc25c182688c49ec6049756fda9786e98e19404baf568753f5f1db10"_unhex;
    expectEncodeAndReencode(signed_response, go_encoded);

    verify(signed_response);
  }

}  // namespace fc::markets::storage
