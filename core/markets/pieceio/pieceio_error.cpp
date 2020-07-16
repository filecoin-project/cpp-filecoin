/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/pieceio/pieceio_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::pieceio, PieceIOError, e) {
  using fc::markets::pieceio::PieceIOError;
  switch (e) {
    case PieceIOError::kCannotCreatePipe:
      return "PieceIOError: cannot create pipe";
    case PieceIOError::kCannotWritePipe:
      return "PieceIOError: cannot write to pipe";
    case PieceIOError::kCannotClosePipe:
      return "PieceIOError: cannot close pipe";
    default:
      return "Unknown error";
  }
}
