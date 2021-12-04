/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_codec.hpp"
#include "common/hexutil.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

/** Check that:
 * - CBOR encoding value yields expected bytes;
 * - CBOR encoding value decoded from bytes yields same bytes;
 */
template <typename T>
void expectEncodeAndReencode(const T &value,
                             const std::vector<uint8_t> &expected_bytes) {
  using fc::common::hex_upper;

  EXPECT_OUTCOME_TRUE(actual_bytes, fc::codec::cbor::encode(value));
  EXPECT_EQ(actual_bytes, expected_bytes)
      << "actual bytes: " << hex_upper(actual_bytes) << std::endl
      << "expected bytes: " << hex_upper(expected_bytes);

  EXPECT_OUTCOME_TRUE(decoded, fc::codec::cbor::decode<T>(expected_bytes));
  EXPECT_OUTCOME_EQ(fc::codec::cbor::encode(decoded), expected_bytes);
}
