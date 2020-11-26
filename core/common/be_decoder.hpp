/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_BE_DECODER_HPP
#define CPP_FILECOIN_CORE_COMMON_BE_DECODER_HPP

#include "common/blob.hpp"

namespace fc::common {

  uint64_t decodeBE(gsl::span<const byte_t> bytes) {
    return static_cast<uint64_t>(bytes[7])
           | (static_cast<uint64_t>(bytes[6]) << 8)
           | (static_cast<uint64_t>(bytes[5]) << 16)
           | (static_cast<uint64_t>(bytes[4]) << 24)
           | (static_cast<uint64_t>(bytes[3]) << 32)
           | (static_cast<uint64_t>(bytes[2]) << 40)
           | (static_cast<uint64_t>(bytes[1]) << 48)
           | (static_cast<uint64_t>(bytes[0]) << 56);
  }

}  // namespace fc::common

#endif  // CPP_FILECOIN_CORE_COMMON_BE_DECODER_HPP