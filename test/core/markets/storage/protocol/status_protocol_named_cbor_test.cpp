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
  class DealStatusProtocolTest : public ProtocolNamedCborTestFixture {
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
  TEST_F(DealStatusProtocolTest, DealStatusRequestCborNamedDecodeFromGo) {
    DealStatusRequestV1_1_0 request;
    request.proposal = cid;
    sign(request);

    const auto go_encoded = normalizeMap(
        "a26850726f706f73616cd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a695369676e6174757265586102813f191c43c7a8d0822e8c5ec8cc9e8c8f01655e8e80e800087fa968e55b88919cebbdbb543e20ecc3f3a29b2a66fa2d0577ed97ea6892590f4a5b47da745d99b4b7882d04f744b0a6280f336579b8de3a3ca83fba16ed0ab6ce3d5242ee2a23"_unhex);
    expectEncodeAndReencode(request, go_encoded);

    verify(request);
  }

  /**
   * @given DealStatusResponse encoded in go-fil-markets implementation
   * @when decode it
   * @then DealStatusResponse is decoded and expected values present
   */
  TEST_F(DealStatusProtocolTest, DealStatusResponseCborNamedDecodeFromGo) {
    DealStatusResponseV1_1_0 deal_status_response(provider_deal_state);
    sign(deal_status_response);

    const auto go_encoded = normalizeMap(
        "a2694465616c5374617465a865537461746500674d657373616765676d6573736167656850726f706f73616c8bd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a190100f555024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc55024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc656c6162656c18651907d24200164300014d4300115c6b50726f706f73616c436964d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6b41646446756e6473436964d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6a5075626c697368436964d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a664465616c4944182a6d4661737452657472696576616cf5695369676e6174757265586102a533b8cfcbd0725c731095ede28f8ea3927af07186ed87a6711eb3059fc86a5796143293f86842239026b8cef7f65f0f03e507e9a35a9ca2535bf71e442f1b16a49fd394b49dd33ba6d92561d4c9cf56b1fb9d7883a2eb4909096d9454d65143"_unhex);
    expectEncodeAndReencode(deal_status_response, go_encoded);

    verify(deal_status_response);
  }

}  // namespace fc::markets::storage
