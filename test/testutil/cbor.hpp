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

inline void normalizeMap(fc::codec::cbor::CborEncodeStream &es,
                         fc::codec::cbor::CborDecodeStream &ds) {
  using fc::codec::cbor::CborDecodeStream;
  using fc::codec::cbor::CborEncodeStream;

  if (ds.isList()) {
    const auto n{ds.listLength()};
    auto dl{ds.list()};
    auto el{CborEncodeStream::list()};
    for (size_t i{}; i != n; ++i) {
      normalizeMap(el, dl);
    }
    es << el;
  } else if (ds.isMap()) {
    auto dm{ds.map()};
    std::map<std::string, CborEncodeStream> em;
    for (auto &[k, v] : dm) {
      normalizeMap(em[k], v);
    }
    es << em;
  } else {
    es << CborEncodeStream::wrap(ds.raw(), 1);
  }
}

/**
 * Reorders cbor map alphabetically.
 * Cbor maps doesn't preserve order and Filecoin implementation orders key
 * alphabetically while Lotus implementation preserves initial order. This
 * method should be called prior the comparison of cbored bytes from Lotus.
 * @param bytes - input cbored map
 * @return - cbor with all maps in alphabetic order
 */
inline fc::Bytes normalizeMap(fc::BytesIn bytes) {
  fc::codec::cbor::CborDecodeStream ds{bytes};
  fc::codec::cbor::CborEncodeStream es;
  normalizeMap(es, ds);
  return es.data();
}
