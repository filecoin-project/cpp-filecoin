/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PIECE_HPP
#define CPP_FILECOIN_PIECE_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::primitives::piece {

  class PaddedPieceSize;

  class UnpaddedPieceSize {
   public:
    UnpaddedPieceSize();

    explicit UnpaddedPieceSize(uint64_t size);

    operator uint64_t() const;  // NOLINT

    UnpaddedPieceSize &operator=(uint64_t rhs);

    PaddedPieceSize padded() const;

    outcome::result<void> validate() const;

   private:
    uint64_t size_;
  };
  CBOR_ENCODE(UnpaddedPieceSize, v) {
    return s << static_cast<uint64_t>(v);
  }
  CBOR_DECODE(UnpaddedPieceSize, v) {
    uint64_t num;
    s >> num;
    v = num;
    return s;
  }

  class PaddedPieceSize {
   public:
    PaddedPieceSize();

    explicit PaddedPieceSize(uint64_t size);

    operator uint64_t() const;  // NOLINT

    PaddedPieceSize &operator=(uint64_t rhs);

    UnpaddedPieceSize unpadded() const;

    outcome::result<void> validate() const;

   private:
    uint64_t size_;
  };
  CBOR_ENCODE(PaddedPieceSize, v) {
    return s << static_cast<uint64_t>(v);
  }
  CBOR_DECODE(PaddedPieceSize, v) {
    uint64_t num;
    s >> num;
    v = num;
    return s;
  }

  class PieceInfo {
   public:
    PieceInfo(const PaddedPieceSize &size, CID piece_CID);

    PaddedPieceSize size;
    CID cid;
  };

};      // namespace fc::primitives::piece
#endif  // CPP_FILECOIN_PIECE_HPP
