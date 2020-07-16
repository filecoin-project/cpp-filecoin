/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_ERROR_HPP
#define CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::markets::pieceio {

  enum class PieceIOError {
    kCannotCreatePipe = 1,
    kCannotWritePipe,
    kCannotClosePipe
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::markets::pieceio, PieceIOError);

#endif  // CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_ERROR_HPP
