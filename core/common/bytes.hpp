/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <gsl/span>
#include <vector>

namespace fc {
  using Bytes = std::vector<uint8_t>;
  using BytesIn = gsl::span<const uint8_t>;
  using BytesOut = gsl::span<uint8_t>;

  template <size_t N>
  using BytesN = std::array<uint8_t, N>;

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
