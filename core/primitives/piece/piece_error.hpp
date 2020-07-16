/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PIECE_ERROR_HPP
#define CPP_FILECOIN_PIECE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::primitives::piece {

  /**
   * @brief Pieces returns these types of errors
   */
  enum class PieceError {
    kLessThatMinimumSize = 1,
    kLessThatMinimumPaddedSize,
    kInvalidUnpaddedSize,
    kInvalidPaddedSize,
  };

}  // namespace fc::primitives::piece

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::piece, PieceError);

#endif  // CPP_FILECOIN_PIECE_ERROR_HPP
