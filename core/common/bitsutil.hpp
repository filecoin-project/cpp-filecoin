/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_BITSUTIL_HPP
#define CPP_FILECOIN_CORE_COMMON_BITSUTIL_HPP

namespace fc::common {
  uint64_t countTrailingZeros(uint64_t n) {
    unsigned int count = 0;
    while ((n & 1) == 0) {
      count += 1;
      n >>= 1;
    }
    return count;
  }

  uint64_t countSetBits(uint64_t n) {
    unsigned int count = 0;
    while (n) {
      count += n & 1;
      n >>= 1;
    }
    return count;
  }

}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_BITSUTIL_HPP
