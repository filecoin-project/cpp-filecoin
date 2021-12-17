/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/fmt/fmt.h>
#include <system_error>

/**
 * std::error_code error;
 * fmt::format("... {}", error); // "... CATEGORY:VALUE"
 * fmt::format("... {:#}", error); // "... CATEGORY error VALUE \"MESSAGE\""
 */
template <>
struct fmt::formatter<std::error_code, char, void> {
  bool alt{false};

  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) {
    const auto *it{ctx.begin()};
    if (it != ctx.end() && *it == '#') {
      alt = true;
      std::advance(it, 1);
    }
    return it;
  }

  template <typename FormatContext>
  auto format(const std::error_code &e, FormatContext &ctx) {
    if (alt) {
      return fmt::format_to(ctx.out(),
                            "{} error {}: \"{}\"",
                            e.category().name(),
                            e.value(),
                            e.message());
    }
    return fmt::format_to(ctx.out(), "{}:{}", e.category().name(), e.value());
  }
};

namespace fc::outcome {
  inline std::string errorToPrettyString(const std::error_code &ec) {
    return fmt::format("{:#}", ec);
  }
}  // namespace fc::outcome
