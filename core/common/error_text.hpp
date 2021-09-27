/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <system_error>

/**
 * only constexpr string literals are allowed
 */
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _ERROR_TEXT_CONST(s)     \
  [] {                           \
    constexpr const char *_s{s}; \
    return _s;                   \
  }()
/**
 * static local variable is used to instantiate only once
 */
#define ERROR_TEXT(s)                                              \
  [] {                                                             \
    static const std::error_code ec{                               \
        ::fc::error_text::_make_error_code(_ERROR_TEXT_CONST(s))}; \
    return ec;                                                     \
  }()

namespace fc::error_text {
  /**
   * prefer statically checked `ERROR_TEXT(s)` to calling this method directly
   */
  std::error_code _make_error_code(const char *message);
}  // namespace fc::error_text
