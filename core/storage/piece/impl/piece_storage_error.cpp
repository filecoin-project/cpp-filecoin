/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/piece/impl/piece_storage_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::piece, PieceStorageError, e) {
  using fc::storage::piece::PieceStorageError;

  switch (e) {
    case PieceStorageError::kPieceNotFound:
      return "PieceStorageError: piece not found";
    case PieceStorageError::kPayloadNotFound:
      return "PieceStorageError: payload not found";
  }

  return "PieceStorageError: unknown error";
}
