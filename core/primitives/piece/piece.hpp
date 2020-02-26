/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PIECE_HPP
#define CPP_FILECOIN_PIECE_HPP

#include "primitives/cid/cid.hpp"

namespace fc::primitives::piece {

  class PaddedPieceSize;

  class UnpaddedPieceSize {
   public:
    UnpaddedPieceSize(uint64_t size);

    operator uint64_t() const;

    UnpaddedPieceSize &operator=(uint64_t rhs);

    PaddedPieceSize padded() const;

    outcome::result<void> validate() const;

   private:
    uint64_t size_;
  };

  class PaddedPieceSize {
   public:
    PaddedPieceSize(uint64_t size);

    operator uint64_t() const;

    PaddedPieceSize &operator=(uint64_t rhs);

    UnpaddedPieceSize unpadded() const;

    outcome::result<void> validate() const;

   private:
    uint64_t size_;
  };

  class PieceInfo {
   public:
    PieceInfo(const PaddedPieceSize &size, CID piece_CID);

    PaddedPieceSize size;
    CID cid;
  };

};      // namespace fc::primitives::piece
#endif  // CPP_FILECOIN_PIECE_HPP
