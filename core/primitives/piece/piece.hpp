/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PIECE_HPP
#define CPP_FILECOIN_PIECE_HPP

#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"

namespace fc::primitives::piece {
  class PieceInfo {
   public:
    PaddedPieceSize size;
    CID piece_CID;
  };

};      // namespace fc::primitives::piece
#endif  // CPP_FILECOIN_PIECE_HPP
