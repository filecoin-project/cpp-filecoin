/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/status_protocol.hpp"
#include "testutil/markets/protocol/protocol_named_cbor_fixture.hpp"

namespace fc::markets::storage {
  using vm::actor::builtin::types::market::ClientDealProposal;

  /**
   * Tests storage market deal status protocol.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   */
  class DealStatusProtocolV1_0_1Test : public ProtocolNamedCborTestFixture {
   public:
    ProviderDealState provider_deal_state{
        StorageDealStatus::STORAGE_DEAL_UNKNOWN,
        "message",
        deal_proposal,
        cid,
        cid,
        cid,
        42,
        true,
    };
  };

  /**
   * @given DealStatusRequest encoded in go-fil-markets implementation
   * @when decode it
   * @then DealStatusRequest is decoded and expected values present
   */
  TEST_F(DealStatusProtocolV1_0_1Test, DealStatusRequestCborNamedDecodeFromGo) {
    DealStatusRequestV1_0_1 request;
    request.proposal = cid;
    sign(request);

    const auto go_encoded =
        "82d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a586102813f191c43c7a8d0822e8c5ec8cc9e8c8f01655e8e80e800087fa968e55b88919cebbdbb543e20ecc3f3a29b2a66fa2d0577ed97ea6892590f4a5b47da745d99b4b7882d04f744b0a6280f336579b8de3a3ca83fba16ed0ab6ce3d5242ee2a23"_unhex;
    expectEncodeAndReencode(request, go_encoded);

    verify(request);
  }

  /**
   * @given DealStatusResponse encoded in go-fil-markets implementation
   * @when decode it
   * @then DealStatusResponse is decoded and expected values present
   */
  TEST_F(DealStatusProtocolV1_0_1Test, DealStatusResponseCborNamedDecodeFromGo) {
    DealStatusResponseV1_0_1 deal_status_response(provider_deal_state);
    sign(deal_status_response);

    const auto go_encoded =
        "828800676d6573736167658bd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a190100f555024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc55024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc656c6162656c18651907d24200164300014d4300115cd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459ad82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459ad82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a182af5586102ae5baa60f6cfbe04fc3b11c45faef42c84ed34581e90cdc0ea01a1e3fc2628d445b469e2937d930f276117892c9f34f61320d56160c6625308262d34f4079e893c8e6a75464394b25471eaa7272144c22d95ceba2ad079aa4548089cd232d1a1"_unhex;
    expectEncodeAndReencode(deal_status_response, go_encoded);

    verify(deal_status_response);
  }

}  // namespace fc::markets::storage
