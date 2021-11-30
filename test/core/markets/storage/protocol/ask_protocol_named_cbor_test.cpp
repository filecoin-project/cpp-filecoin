/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/ask_protocol.hpp"
#include "testutil/markets/protocol/protocol_named_cbor_fixture.hpp"

namespace fc::markets::storage {
  /**
   * Tests storage market ask protocol.
   * Expected encoded bytes are from go-fil-markets implementation (commit:
   * b1a66cfd12686a8af6030fccace49916849b1954).
   * Note: The order of named fields is not determined, so we cannot just
   * compare raw bytes.
   */
  class AskProtocolTest : public ProtocolNamedCborTestFixture {
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
  TEST_F(AskProtocolTest, StorageAskCborNamedDecodeFromGo) {
    StorageAskV1_1_0 storage_ask{expected_storage_ask};
    const auto go_encoded =
        "a865507269636542007b6d56657269666965645072696365430001c86c4d696e506965636553697a651901006c4d6178506965636553697a651a00100000654d696e657255024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc6954696d657374616d701904d266457870697279191a85655365714e6f182a"_unhex;
    expectEncodeAndReencode(storage_ask, go_encoded);
  }

  /**
   * @given StorageAsk encoded in go-fil-markets implementation
   * @when decode it
   * @then signed storage ask is decoded and expected values present
   */
  TEST_F(AskProtocolTest, SignedStorageAskEncodeAndDecode) {
    SignedStorageAskV1_1_0 expected_signed_ask(expected_storage_ask);
    sign(expected_signed_ask);

    const auto go_encoded =
        "a26341736ba865507269636542007b6d56657269666965645072696365430001c86c4d696e506965636553697a651901006c4d6178506965636553697a651a00100000654d696e657255024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc6954696d657374616d701904d266457870697279191a85655365714e6f182a695369676e617475726558610289564aca0dabd06015c1c87816f6aedec346c6c0dad93549503907d97e2147e0b07fb35d29da1477d5e946efbfd5b07708aa8d753060ffdbc244c117e3119d279510ba4b2e0423da649e751b9422cd6d7ee3bbf216a517de6fde0f6eb67640b7"_unhex;
    expectEncodeAndReencode(expected_signed_ask, go_encoded);

    verify(expected_signed_ask);
  }

  /**
   * @given AskRequest encoded in go-fil-markets implementation
   * @when decode it
   * @then decoded and expected values are present
   */
  TEST_F(AskProtocolTest, AskRequestEncodeAndDecode) {
    AskRequestV1_1_0 expected_request{{
        .miner = address,
    }};
    const auto go_encoded =
        "a1654d696e657255024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc"_unhex;
    expectEncodeAndReencode(expected_request, go_encoded);
  }

  /**
   * @given AskResponse encoded in go-fil-markets implementation
   * @when decode it
   * @then decoded and expected values are present
   */
  TEST_F(AskProtocolTest, AskResponseEncodeAndDecode) {
    SignedStorageAskV1_1_0 signed_ask(expected_storage_ask);
    sign(signed_ask);
    AskResponseV1_1_0 expected_response{signed_ask};

    const auto go_encoded =
        "a16341736ba26341736ba865507269636542007b6d56657269666965645072696365430001c86c4d696e506965636553697a651901006c4d6178506965636553697a651a00100000654d696e657255024716b023b7fe84b6e7dcda303c3d754b1a8ff2fc6954696d657374616d701904d266457870697279191a85655365714e6f182a695369676e617475726558610289564aca0dabd06015c1c87816f6aedec346c6c0dad93549503907d97e2147e0b07fb35d29da1477d5e946efbfd5b07708aa8d753060ffdbc244c117e3119d279510ba4b2e0423da649e751b9422cd6d7ee3bbf216a517de6fde0f6eb67640b7"_unhex;
    expectEncodeAndReencode(expected_response, go_encoded);

    verify(expected_response.ask());
  }

}  // namespace fc::markets::storage
