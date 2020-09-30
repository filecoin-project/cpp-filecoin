/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_SPAN_HPP
#define CPP_FILECOIN_CORE_COMMON_SPAN_HPP

#include <gsl/span>

namespace fc::common::span {
  using BytesIn = gsl::span<const uint8_t>;

  template <typename To, typename From>
  constexpr auto cast(From *ptr) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<To *>(ptr);
  }

  template <typename To, typename From>
  constexpr auto cast(gsl::span<From> span) {
    static_assert(sizeof(To) == 1);
    return gsl::make_span(cast<To>(span.data()), span.size_bytes());
  }

  constexpr auto cbytes(gsl::span<const char> span) {
    return cast<const uint8_t>(span);
  }

  constexpr auto cstring(BytesIn span) {
    return cast<const char>(span);
  }

  constexpr auto string(gsl::span<uint8_t> span) {
    return cast<char>(span);
  }

  constexpr auto bytestr(BytesIn span) {
    return std::string_view(cstring(span).data(), span.size());
  }

  constexpr auto bytestr(const uint8_t *ptr) {
    return cast<const char>(ptr);
  }
}  // namespace fc::common::span

#endif  // CPP_FILECOIN_CORE_COMMON_SPAN_HPP
