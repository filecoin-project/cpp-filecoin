/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/std_function_signature.hpp"

namespace fc {
  template <typename T>
  using MockStdFunction = ::testing::MockFunction<std_function_signature_t<T>>;
}  // namespace fc
