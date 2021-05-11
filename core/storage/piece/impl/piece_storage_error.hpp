/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::storage::piece {

  /**
   * @enum Piece storage errors
   */
  enum class PieceStorageError { kPieceNotFound = 1, kPayloadNotFound };

}  // namespace fc::storage::piece

OUTCOME_HPP_DECLARE_ERROR(fc::storage::piece, PieceStorageError);
