/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/primitives/piece/piece_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::piece, PieceError, e) {
  using fc::primitives::piece::PieceError;

  switch (e) {
    case (PieceError::LESS_THAT_MINIMUM_SIZE):
      return "Piece: minimum piece size is 127 bytes";
    case (PieceError::LESS_THAT_MINIMUM_PADDED_SIZE):
      return "Piece: minimum padded piece size is 128 bytes";
    case (PieceError::INVALID_UNPADDED_SIZE):
      return "Piece: unpadded piece size must be a power of 2 multiple of 127";
    case (PieceError::INVALID_PADDED_SIZE):
      return "Piece: padded piece size must be a power of 2";
    default:
      return "Piece: unknown error";
  }
}
