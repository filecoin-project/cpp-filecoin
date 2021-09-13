/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::markets::pieceio {

  enum class PieceIOError {
    kCannotCreatePipe = 1,
    kCannotWritePipe,
    kCannotClosePipe,
    kFileNotExist,
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::markets::pieceio, PieceIOError);
