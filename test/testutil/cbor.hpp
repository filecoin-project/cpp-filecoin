/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_CBOR_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_CBOR_HPP

#include "codec/cbor/cbor.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

/** Check that:
 * - CBOR encoding value yields expected bytes;
 * - CBOR encoding value decoded from bytes yields same bytes;
 */
template <typename T>
void expectEncodeAndReencode(const T &value,
                             const std::vector<uint8_t> &bytes) {
  EXPECT_OUTCOME_EQ(fc::codec::cbor::encode(value), bytes);
  EXPECT_OUTCOME_TRUE(decoded, fc::codec::cbor::decode<T>(bytes));
  EXPECT_OUTCOME_EQ(fc::codec::cbor::encode(decoded), bytes);
}

inline auto operator""_cid(const char *c, size_t s) {
  return libp2p::multi::ContentIdentifierCodec::decode(
             fc::common::unhex(std::string_view(c, s)).value())
      .value();
}

#endif  // CPP_FILECOIN_TEST_TESTUTIL_CBOR_HPP
