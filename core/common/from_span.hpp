/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/error_text.hpp"
#include "common/outcome.hpp"
#include "common/span.hpp"

namespace fc {
  template <typename T>
  inline outcome::result<T> fromSpan(BytesIn input, bool exact = true) {
    auto size{(ptrdiff_t)sizeof(T)};
    if (exact && input.size() != size) {
      return ERROR_TEXT("fromSpan: too much bytes");
    }
    if (input.size() < size) {
      return ERROR_TEXT("fromSpan: not enough bytes");
    }
    return *common::span::cast<const T>(input.data());
  }
}  // namespace fc
