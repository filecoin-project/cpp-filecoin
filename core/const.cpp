/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "const.hpp"

#define DEFINE(x) decltype(x) x

namespace fc {
  DEFINE(kConsensusMinerMinPower){BigInt{1} << 40};
}  // namespace fc
