/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_PIECE_IMPL_PIECE_STORAGE_ERROR_HPP
#define CPP_FILECOIN_CORE_STORAGE_PIECE_IMPL_PIECE_STORAGE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::storage::piece {

  /**
   * @enum Piece storage errors
   */
  enum class PieceStorageError { kPieceNotFound = 1, kPayloadNotFound };

}  // namespace fc::storage::piece

OUTCOME_HPP_DECLARE_ERROR(fc::storage::piece, PieceStorageError);

#endif  // CPP_FILECOIN_CORE_STORAGE_PIECE_IMPL_PIECE_STORAGE_ERROR_HPP
