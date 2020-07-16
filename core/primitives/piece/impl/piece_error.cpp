/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/piece/piece_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::piece, PieceError, e) {
  using fc::primitives::piece::PieceError;

  switch (e) {
    case (PieceError::kLessThatMinimumSize):
      return "Piece: minimum piece size is 127 bytes";
    case (PieceError::kLessThatMinimumPaddedSize):
      return "Piece: minimum padded piece size is 128 bytes";
    case (PieceError::kInvalidUnpaddedSize):
      return "Piece: unpadded piece size must be a power of 2 multiple of 127";
    case (PieceError::kInvalidPaddedSize):
      return "Piece: padded piece size must be a power of 2";
    default:
      return "Piece: unknown error";
  }
}
