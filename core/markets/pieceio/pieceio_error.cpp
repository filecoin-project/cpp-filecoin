/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/pieceio/pieceio_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::pieceio, PieceIOError, e) {
  using fc::markets::pieceio::PieceIOError;
  switch (e) {
    case PieceIOError::CANNOT_CREATE_PIPE:
      return "PieceIOError: cannot create pipe";
    case PieceIOError::CANNOT_WRITE_PIPE:
      return "PieceIOError: cannot write to pipe";
    case PieceIOError::CANNOT_CLOSE_PIPE:
      return "PieceIOError: cannot close pipe";
    case PieceIOError::PAYLOAD_NOT_FOUND:
      return "PieceIOError: Piece, containing selected payload, not found";
    case PieceIOError::PAYLOAD_READ_ERROR:
      return "PieceIOError: error reading payload block from the Piece";
    default:
      return "Unknown error";
  }
}
