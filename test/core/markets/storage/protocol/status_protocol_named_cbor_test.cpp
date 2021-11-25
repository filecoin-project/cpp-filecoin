/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "markets/storage/status_protocol.hpp"
#include "testutil/cbor.hpp"
#include "testutil/outcome.hpp"

namespace fc::markets::storage {
  using primitives::address::Address;
  using primitives::address::decodeFromString;
  using primitives::piece::PaddedPieceSize;
  using vm::actor::builtin::types::market::ClientDealProposal;
  using vm::actor::builtin::types::market::DealProposal;

  /**
   * Tests storage market deal status protocol.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   * Note: The order of named fields is not determined, so we cannot just
   * compare raw bytes.
   */
  class DealProtocolTest : public ::testing::Test {
   public:
    // address from go test constants
    Address address =
        decodeFromString("t2i4llai5x72clnz643iydyplvjmni74x4vyme7ny").value();
    // CID from go-generated string
    CID cid = CID::fromString("QmTTA2daxGqo5denp6SwLzzkLJm3fuisYEi9CoWsuHpzfb")
                  .value();
    crypto::signature::Signature signature{
        crypto::signature::Secp256k1Signature{}};

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

    ProviderDealStateV1_1_0 provider_deal_state{
        StorageDealStatus::STORAGE_DEAL_UNKNOWN,
        "message",
        deal_proposal,
        cid,
        cid,
        cid,
        42,
        true,
    };

    DealStatusResponseV1_1_0 deal_status_response{
        provider_deal_state,
        signature,
    };

    template <typename T>
    T encodeAndDecodeNamed(const T &value) {
      EXPECT_OUTCOME_TRUE(encoded, fc::codec::cbor::encode(value));
      EXPECT_OUTCOME_TRUE(decoded, fc::codec::cbor::decode<T>(encoded));
      return decoded;
    }

    /** Ensures the deal_status_response has expected values */
    void expectDealStatusResponse(const DealStatusResponse &response) {
      EXPECT_EQ(response.state().status,
                StorageDealStatus::STORAGE_DEAL_UNKNOWN);
      EXPECT_EQ(response.state().message, "message");
      EXPECT_EQ(response.state().proposal.piece_cid, cid);
      EXPECT_EQ(response.state().proposal.piece_size, PaddedPieceSize{256});
      EXPECT_EQ(response.state().proposal.verified, true);
      EXPECT_EQ(response.state().proposal.client, address);
      EXPECT_EQ(response.state().proposal.provider, address);
      EXPECT_EQ(response.state().proposal.label, "label");
      EXPECT_EQ(response.state().proposal.start_epoch, 101);
      EXPECT_EQ(response.state().proposal.end_epoch, 2002);
      EXPECT_EQ(response.state().proposal.storage_price_per_epoch, 22);
      EXPECT_EQ(response.state().proposal.provider_collateral, 333);
      EXPECT_EQ(response.state().proposal.client_collateral, 4444);
      EXPECT_EQ(response.state().proposal_cid, cid);
      EXPECT_EQ(*response.state().add_funds_cid, cid);
      EXPECT_EQ(*response.state().publish_cid, cid);
      EXPECT_EQ(response.state().id, 42);
      EXPECT_EQ(response.state().fast_retrieval, true);
      EXPECT_TRUE(response.signature == signature);
    }
  };

  /**
   * Encode and decode, must be equal
   */
  TEST_F(DealProtocolTest, DealStatusRequestEncodeAndDecode) {
    DealStatusRequestV1_1_0 request{cid, signature};
    auto decoded = encodeAndDecodeNamed<DealStatusRequestV1_1_0>(request);
    EXPECT_EQ(decoded.proposal, request.proposal);
    EXPECT_TRUE(decoded.signature == request.signature);
  }

  /**
   * @given DealStatusRequest encoded in go-fil-markets implementation
   * @when decode it
   * @then DealStatusRequest is decoded and expected values present
   */
  TEST_F(DealProtocolTest, DealStatusRequestCborNamedDecodeFromGo) {
    const auto go_encoded =
        "a26850726f706f73616cd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a695369676e61747572655842010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"_unhex;

    EXPECT_OUTCOME_TRUE(
        decoded, codec::cbor::decode<DealStatusRequestV1_1_0>(go_encoded));
    EXPECT_EQ(decoded.proposal, cid);
    EXPECT_TRUE(decoded.signature == signature);
  }

  /**
   * Encode and decode, must be equal
   */
  TEST_F(DealProtocolTest, DealStatusResponseEncodeAndDecode) {
    auto decoded =
        encodeAndDecodeNamed<DealStatusResponseV1_1_0>(deal_status_response);
    expectDealStatusResponse(decoded);
  }

  /**
   * @given DealStatusResponse encoded in go-fil-markets implementation
   * @when decode it
   * @then DealStatusResponse is decoded and expected values present
   */
  TEST_F(DealProtocolTest, DealStatusResponseCborNamedDecodeFromGo) {
    const auto go_encoded =
        "a2694465616c5374617465a865537461746500674d657373616765676d6573736167656850726f706f73616c8bd82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a190100f555024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc55024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc656c6162656c18651907d24200164300014d4300115c6b50726f706f73616c436964d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6b41646446756e6473436964d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a6a5075626c697368436964d82a58230012204bf5122f344554c53bde2ebb8cd2b7e3d1600ad631c385a5d7cce23c7785459a664465616c4944182a6d4661737452657472696576616cf5695369676e61747572655842010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"_unhex;

    EXPECT_OUTCOME_TRUE(
        decoded, codec::cbor::decode<DealStatusResponseV1_1_0>(go_encoded));
    expectDealStatusResponse(decoded);
  }

}  // namespace fc::markets::storage
