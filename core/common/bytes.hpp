/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <gsl/span>
#include <vector>
#include "common/hexutil.hpp"

namespace fc {
  using Bytes = std::vector<uint8_t>;
  using BytesIn = gsl::span<const uint8_t>;
  using BytesOut = gsl::span<uint8_t>;

  template <size_t N>
  using BytesN = std::array<uint8_t, N>;

  struct BytesLess {
    using is_transparent = void;
    constexpr bool operator()(const BytesIn &l, const BytesIn &r) const {
      return l < r;
    }
  };

  inline outcome::result<Bytes> fromHex(std::string_view s) {
    return common::unhex(s);
  }

  inline std::string toHex(BytesIn b) {
    return common::hex_upper(b);
  }

  inline std::string toHex(Bytes b) {
    return toHex(gsl::make_span(b));
  }

  inline Bytes copy(BytesIn r) {
    return {r.begin(), r.end()};
  }
  void copy(Bytes &&) = delete;

  inline void copy(Bytes &l, BytesIn r) {
    l.assign(r.begin(), r.end());
  }

  inline void append(Bytes &l, BytesIn r) {
    l.insert(l.end(), r.begin(), r.end());
  }

  inline void putUint64(Bytes &l, uint64_t n) {
    l.push_back(static_cast<unsigned char &&>((n >> 56u) & 0xFF));
    l.push_back(static_cast<unsigned char &&>((n >> 48u) & 0xFF));
    l.push_back(static_cast<unsigned char &&>((n >> 40u) & 0xFF));
    l.push_back(static_cast<unsigned char &&>((n >> 32u) & 0xFF));
    l.push_back(static_cast<unsigned char &&>((n >> 24u) & 0xFF));
    l.push_back(static_cast<unsigned char &&>((n >> 16u) & 0xFF));
    l.push_back(static_cast<unsigned char &&>((n >> 8u) & 0xFF));
    l.push_back(static_cast<unsigned char &&>((n)&0xFF));
  }
}  // namespace fc

namespace gsl {
  inline bool operator==(const fc::Bytes &l, const fc::BytesIn &r) {
    return fc::BytesIn{l} == r;
  }
  inline bool operator==(const fc::BytesIn &l, const fc::Bytes &r) {
    return l == fc::BytesIn{r};
  }

  inline bool operator<(const fc::Bytes &l, const fc::BytesIn &r) {
    return fc::BytesIn{l} < r;
  }
  inline bool operator<(const fc::BytesIn &l, const fc::Bytes &r) {
    return l < fc::BytesIn{r};
  }
}  // namespace gsl
