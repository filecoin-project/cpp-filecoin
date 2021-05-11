/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gsl/span>
#include <optional>

namespace fc {
  using BytesIn = gsl::span<const uint8_t>;

  constexpr bool read(BytesIn &out, BytesIn &input, size_t n) {
    if ((size_t)input.size() < n) {
      out = {};
      return false;
    }
    out = input.subspan(0, n);
    input = input.subspan(n);
    return true;
  }

  constexpr std::optional<BytesIn> read(BytesIn &input, size_t n) {
    BytesIn out;
    if (read(out, input, n)) {
      return out;
    }
    return {};
  }

  constexpr bool readPrefix(BytesIn &input, BytesIn expected) {
    auto n{expected.size()};
    return input.size() >= n && input.subspan(0, n) == expected
           && read(input, n);
  }
}  // namespace fc
