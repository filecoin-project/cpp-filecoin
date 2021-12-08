/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/ask_protocol.hpp"
#include "testutil/markets/protocol/protocol_named_cbor_fixture.hpp"

namespace fc::markets::storage {
  /**
   * Tests storage market ask protocol v1.0.1.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   */
  class AskProtocolV1_0_1Test : public ProtocolNamedCborTestFixture {
   public:
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
  };

  /**
   * @given StorageAsk encoded in go-fil-markets implementation
   * @when decode it
   * @then storage ask is decoded and expected values present
   */
  TEST_F(AskProtocolV1_0_1Test, StorageAskCborNamedDecodeFromGo) {
    StorageAskV1_0_1 storage_ask{expected_storage_ask};
    const auto go_encoded =
        "8842007b430001c81901001a0010000055024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc1904d2191a85182a"_unhex;
    expectEncodeAndReencode(storage_ask, go_encoded);
  }

  /**
   * @given StorageAsk encoded in go-fil-markets implementation
   * @when decode it
   * @then signed storage ask is decoded and expected values present
   */
  TEST_F(AskProtocolV1_0_1Test, SignedStorageAskEncodeAndDecode) {
    SignedStorageAskV1_0_1 expected_signed_ask(expected_storage_ask);
    sign(expected_signed_ask);

    const auto go_encoded =
        "828842007b430001c81901001a0010000055024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc1904d2191a85182a586102b98790ac7d59a4b95344633b9dd20f9afcbb7e825836c9ed2d6861d129e5c47749c1720b53f6fbd952d9b79b69b4c8e60f3c7375c03ff98718f66335ae41c9aec3b548742ed7ad67c79a20371b752c2b05fe909928f212d8f60396a7484725c8"_unhex;
    expectEncodeAndReencode(expected_signed_ask, go_encoded);

    verify(expected_signed_ask);
  }

  /**
   * @given AskRequest encoded in go-fil-markets implementation
   * @when decode it
   * @then decoded and expected values are present
   */
  TEST_F(AskProtocolV1_0_1Test, AskRequestEncodeAndDecode) {
    AskRequestV1_0_1 expected_request{{
        .miner = address,
    }};
    const auto go_encoded =
        "8155024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc"_unhex;
    expectEncodeAndReencode(expected_request, go_encoded);
  }

  /**
   * @given AskResponse encoded in go-fil-markets implementation
   * @when decode it
   * @then decoded and expected values are present
   */
  TEST_F(AskProtocolV1_0_1Test, AskResponseEncodeAndDecode) {
    SignedStorageAskV1_0_1 signed_ask(expected_storage_ask);
    sign(signed_ask);
    AskResponseV1_0_1 expected_response{signed_ask};

    const auto go_encoded =
        "81828842007b430001c81901001a0010000055024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc1904d2191a85182a586102b98790ac7d59a4b95344633b9dd20f9afcbb7e825836c9ed2d6861d129e5c47749c1720b53f6fbd952d9b79b69b4c8e60f3c7375c03ff98718f66335ae41c9aec3b548742ed7ad67c79a20371b752c2b05fe909928f212d8f60396a7484725c8"_unhex;
    expectEncodeAndReencode(expected_response, go_encoded);

    verify(expected_response.ask());
  }

}  // namespace fc::markets::storage
