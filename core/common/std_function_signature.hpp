/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>

namespace fc {
  template <typename T>
  struct std_function_signature {};

  template <typename T>
  struct std_function_signature<std::function<T>> {
    using type = T;
  };

  template <typename T>
  using std_function_signature_t = typename std_function_signature<T>::type;
}  // namespace fc
