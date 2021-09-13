/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::codec::rle {

  namespace errors {

    struct VersionMismatch : public std::exception {};

    struct UnpackBytesOverflow : public std::exception {};

    struct MaxSizeExceed : public std::exception {};

  }  // namespace errors

  /**
   * @class RLE+ decode errors
   */
  enum class RLEPlusDecodeError : int {
    kVersionMismatch = 1, /**< RLE+ data header has invalid version */
    kUnpackOverflow,      /**< RLE+ invalid encoding */
    kMaxSizeExceed        /**< RLE+ object size too large */
  };
}  // namespace fc::codec::rle

OUTCOME_HPP_DECLARE_ERROR(fc::codec::rle, RLEPlusDecodeError);
