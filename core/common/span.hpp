/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_SPAN_HPP
#define CPP_FILECOIN_CORE_COMMON_SPAN_HPP

#include <gsl/span>

namespace fc::common::span {
  namespace {
    template <typename To, typename From>
    constexpr auto cast(gsl::span<From> span) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return gsl::make_span(reinterpret_cast<To *>(span.data()), span.size());
    }
  }  // namespace

  constexpr auto cbytes(gsl::span<const char> span) {
    return cast<const uint8_t>(span);
  }

  constexpr auto cstring(gsl::span<const uint8_t> span) {
    return cast<const char>(span);
  }

  constexpr auto string(gsl::span<uint8_t> span) {
    return cast<char>(span);
  }
}  // namespace fc::common::span

#endif  // CPP_FILECOIN_CORE_COMMON_SPAN_HPP
