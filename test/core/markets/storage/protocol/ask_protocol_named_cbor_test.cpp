/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "markets/storage/ask_protocol.hpp"
#include "testutil/cbor.hpp"
#include "testutil/outcome.hpp"

namespace fc::markets::storage {
  using primitives::address::Address;
  using primitives::address::decodeFromString;
  using primitives::piece::PaddedPieceSize;

  /**
   * Tests storage market ask protocol.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   * Note: The order of named fields is not determined, so we cannot just
   * compare raw bytes.
   */
  class AskProtocolTest : public ::testing::Test {
   public:
    // address from go test constants
    Address address =
        decodeFromString("t2i4llai5x72clnz643iydyplvjmni74x4vyme7ny").value();
    // storage ask used in go tests
    StorageAsk expected_storage_ask{
        .price = 123,
        .verified_price = 456,
        .min_piece_size = PaddedPieceSize{256},
        .max_piece_size = PaddedPieceSize{1 << 20},
        .miner = address,
        .timestamp = 1234,
        .expiry = 6789,
        .seq_no = 42,
    };
    crypto::signature::Signature signature{
        crypto::signature::Secp256k1Signature{}};

    void expectStorageAsksEqual(const StorageAsk &lhs, const StorageAsk &rhs) {
      EXPECT_EQ(lhs.price, rhs.price);
      EXPECT_EQ(lhs.verified_price, rhs.verified_price);
      EXPECT_EQ(lhs.min_piece_size, rhs.min_piece_size);
      EXPECT_EQ(lhs.max_piece_size, rhs.max_piece_size);
      EXPECT_EQ(lhs.miner, rhs.miner);
      EXPECT_EQ(lhs.timestamp, rhs.timestamp);
      EXPECT_EQ(lhs.expiry, rhs.expiry);
      EXPECT_EQ(lhs.seq_no, rhs.seq_no);
    }

    template <typename T>
    T encodeAndDecodeNamed(const T &value) {
      using Named = typename T::Named;
      EXPECT_OUTCOME_TRUE(encoded, fc::codec::cbor::encode(Named{value}));
      EXPECT_OUTCOME_TRUE(decoded, fc::codec::cbor::decode<Named>(encoded));
      return decoded;
    }
  };

  /**
   * Encode and decode, must be equal
   */
  TEST_F(AskProtocolTest, StorageAskEncodeAndDecode) {
    auto decoded = encodeAndDecodeNamed<StorageAsk>(expected_storage_ask);
    expectStorageAsksEqual(decoded, expected_storage_ask);
  }

  /**
   * @given StorageAsk encoded in go-fil-markets implementation
   * @when decode it
   * @then storage ask is decoded and expected values present
   */
  TEST_F(AskProtocolTest, StorageAskCborNamedDecodeFromGo) {
    const auto go_encoded =
        "a865507269636542007b6d56657269666965645072696365430001c86c4d696e506965636553697a651901006c4d6178506965636553697a651a00100000654d696e657255024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc6954696d657374616d701904d266457870697279191a85655365714e6f182a"_unhex;

    EXPECT_OUTCOME_TRUE(decoded,
                        fc::codec::cbor::decode<StorageAsk::Named>(go_encoded));
    expectStorageAsksEqual(decoded, expected_storage_ask);
  }

  /**
   * Encode and decode, must be equal
   */
  TEST_F(AskProtocolTest, SignedStorageAskEncodeAndDecode) {
    SignedStorageAsk expected_signed_ask{
        .ask = expected_storage_ask,
        .signature = signature,
    };
    auto decoded = encodeAndDecodeNamed<SignedStorageAsk>(expected_signed_ask);
    expectStorageAsksEqual(decoded.ask, expected_storage_ask);
    EXPECT_TRUE(decoded.signature == signature);
  }

  /**
   * @given StorageAsk encoded in go-fil-markets implementation
   * @when decode it
   * @then signed storage ask is decoded and expected values present
   */
  TEST_F(AskProtocolTest, SignedStorageAskCborNamedDecodeFromGo) {
    auto go_encoded =
        "a26341736ba865507269636542007b6d56657269666965645072696365430001c86c4d696e506965636553697a651901006c4d6178506965636553697a651a00100000654d696e657255024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc6954696d657374616d701904d266457870697279191a85655365714e6f182a695369676e61747572655842010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"_unhex;

    EXPECT_OUTCOME_TRUE(
        decoded, fc::codec::cbor::decode<SignedStorageAsk::Named>(go_encoded));

    EXPECT_TRUE(decoded.signature == signature);
    expectStorageAsksEqual(decoded.ask, expected_storage_ask);
  }

  /**
   * Encode and decode, must be equal
   */
  TEST_F(AskProtocolTest, AskRequestEncodeAndDecode) {
    AskRequest expected_request{
        .miner = address,
    };
    auto decoded = encodeAndDecodeNamed<AskRequest>(expected_request);
    EXPECT_EQ(decoded.miner, expected_request.miner);
  }

  /**
   * @given AskRequest encoded in go-fil-markets implementation
   * @when decode it
   * @then decoded and expected values are present
   */
  TEST_F(AskProtocolTest, AskRequestCborNamedDecodeFromGo) {
    auto go_encoded =
        "a1654d696e657255024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc"_unhex;

    EXPECT_OUTCOME_TRUE(decoded,
                        fc::codec::cbor::decode<AskRequest::Named>(go_encoded));
    EXPECT_EQ(decoded.miner, address);
  }

  /**
   * Encode and decode, must be equal
   */
  TEST_F(AskProtocolTest, AskResponseEncodeAndDecode) {
    AskResponse expected_response{.ask = SignedStorageAsk{
                                      .ask = expected_storage_ask,
                                      .signature = signature,
                                  }};
    auto decoded = encodeAndDecodeNamed<AskResponse>(expected_response);
    EXPECT_TRUE(decoded.ask.signature == signature);
    expectStorageAsksEqual(decoded.ask.ask, expected_storage_ask);
  }

  /**
   * @given AskResponse encoded in go-fil-markets implementation
   * @when decode it
   * @then decoded and expected values are present
   */
  TEST_F(AskProtocolTest, AskResponseCborNamedDecodeFromGo) {
    auto go_encoded =
        "a16341736ba26341736ba865507269636542007b6d56657269666965645072696365430001c86c4d696e506965636553697a651901006c4d6178506965636553697a651a00100000654d696e657255024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc6954696d657374616d701904d266457870697279191a85655365714e6f182a695369676e61747572655842010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"_unhex;

    EXPECT_OUTCOME_TRUE(
        decoded, fc::codec::cbor::decode<AskResponse::Named>(go_encoded));
    EXPECT_TRUE(decoded.ask.signature == signature);
    expectStorageAsksEqual(decoded.ask.ask, expected_storage_ask);
  }
}  // namespace fc::markets::storage
