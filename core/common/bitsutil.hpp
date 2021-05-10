/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace fc::common {
  inline uint64_t countTrailingZeros(uint64_t n) {
    if (n == 0) return 64;

    unsigned int count = 0;
    while ((n & 1) == 0) {
      count += 1;
      n >>= 1;
    }
    return count;
  }

  inline uint64_t countSetBits(uint64_t n) {
    unsigned int count = 0;
    while (n != 0) {
      count += n & 1;
      n >>= 1;
    }
    return count;
  }

}  // namespace fc::common
