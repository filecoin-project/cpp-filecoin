/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/multiprecision/cpp_int.hpp>

#include "codec/cbor/streams_annotation.hpp"

namespace fc::primitives {
  using BigInt = boost::multiprecision::cpp_int;

  /// div with round-floor, like `big.Div` from go
  inline BigInt bigdiv(const BigInt &n, const BigInt &d) {
    if (!n.is_zero() && n.sign() != d.sign()) {
      return (n + 1) / d - 1;
    }
    return n / d;
  }

  /// mod with round-floor, like `big.Mod` from go
  inline BigInt bigmod(const BigInt &n, const BigInt &d) {
    const auto r = bigdiv(n, d);
    return n - r * d;
  }

}  // namespace fc::primitives

namespace fc {
  using primitives::bigdiv;
  using primitives::bigmod;
  using primitives::BigInt;

  inline auto bitlen(const BigInt &x) {
    return x ? msb(x < 0 ? -x : x) : 0;
  }
}  // namespace fc

namespace boost::multiprecision {
  CBOR_ENCODE(cpp_int, big_int) {
    std::vector<uint8_t> bytes;
    if (big_int != 0) {
      bytes.push_back(big_int < 0 ? 1 : 0);
      export_bits(big_int, std::back_inserter(bytes), 8);
    }
    return s << bytes;
  }

  CBOR_DECODE(cpp_int, big_int) {
    std::vector<uint8_t> bytes;
    s >> bytes;
    if (bytes.empty()) {
      big_int = 0;
    } else {
      import_bits(big_int, bytes.begin() + 1, bytes.end());
      if (bytes[0] == 1) {
        big_int = -big_int;
      }
    }
    return s;
  }
}  // namespace boost::multiprecision
