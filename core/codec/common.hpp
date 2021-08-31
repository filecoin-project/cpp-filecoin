/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gsl/span>
#include <optional>

#include "common/bytes.hpp"

namespace fc::codec {
  constexpr bool read(BytesIn &out, BytesIn &input, size_t n) {
    if (static_cast<size_t>(input.size()) < n) {
      out = {};
      return false;
    }
    out = input.subspan(0, static_cast<gsl::span<BytesIn>::index_type>(n));
    input = input.subspan(static_cast<gsl::span<BytesIn>::index_type>(n));
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
}  // namespace fc::codec
