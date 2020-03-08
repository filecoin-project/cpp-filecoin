/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_CODEC_RLE_PLUS_ERRORS_HPP
#define CORE_CODEC_RLE_PLUS_ERRORS_HPP

#include "filecoin/common/outcome.hpp"

namespace fc::codec::rle {

  namespace errors {

    struct VersionMismatch : public std::exception {};

    struct IndexOutOfBound : public std::exception {};

    struct UnpackBytesOverflow : public std::exception {};

    struct MaxSizeExceed : public std::exception {};

  }  // namespace errors

  /**
   * @class RLE+ decode errors
   */
  enum class RLEPlusDecodeError : int {
    VersionMismatch = 1, /**< RLE+ data header has invalid version */
    DataIndexFailure,    /**< RLE+ incorrect structure */
    UnpackOverflow,      /**< RLE+ invalid encoding */
    MaxSizeExceed        /**< RLE+ object size too large */
  };
}  // namespace fc::codec::rle

OUTCOME_HPP_DECLARE_ERROR(fc::codec::rle, RLEPlusDecodeError);

#endif
