/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_ERROR_HPP
#define CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::markets::pieceio {

  enum class PieceIOError {
    CANNOT_CREATE_PIPE = 1,
    CANNOT_WRITE_PIPE,
    CANNOT_CLOSE_PIPE,
    PAYLOAD_NOT_FOUND,
    PAYLOAD_READ_ERROR
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::markets::pieceio, PieceIOError);

#endif  // CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_ERROR_HPP
