/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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

    UnpaddedPieceSize &operator+=(uint64_t rhs);

    PaddedPieceSize padded() const;

    outcome::result<void> validate() const;

   private:
    uint64_t size_;
  };
  CBOR_ENCODE(UnpaddedPieceSize, v) {
    return s << static_cast<uint64_t>(v);
  }
  CBOR_DECODE(UnpaddedPieceSize, v) {
    uint64_t num = 0;
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

    PaddedPieceSize &operator+=(uint64_t rhs);

    UnpaddedPieceSize unpadded() const;

    outcome::result<void> validate() const;

   private:
    uint64_t size_;
  };
  CBOR_ENCODE(PaddedPieceSize, v) {
    return s << static_cast<uint64_t>(v);
  }
  CBOR_DECODE(PaddedPieceSize, v) {
    uint64_t num = 0;
    s >> num;
    v = num;
    return s;
  }

  struct PieceInfo {
    PaddedPieceSize size;
    CID cid;
  };
  CBOR_TUPLE(PieceInfo, size, cid)

  inline bool operator==(const PieceInfo &lhs, const PieceInfo &rhs) {
    return lhs.size == rhs.size && lhs.cid == rhs.cid;
  }

  /**
   * @brief PaddedSize takes size to the next power of two and then returns the
   * number of not-bit-padded bytes that would fit into a sector of that size.
   */
  UnpaddedPieceSize paddedSize(uint64_t size);

  using UnpaddedByteIndex = uint64_t;
  using PaddedByteIndex = uint64_t;

  PaddedByteIndex paddedIndex(UnpaddedByteIndex index);
  UnpaddedByteIndex unpaddedIndex(PaddedByteIndex index);

  const uint64_t kMultithreadingTreshold = uint64_t(32) << 20;

  void pad(gsl::span<const uint8_t> in, gsl::span<uint8_t> out);
  void unpad(gsl::span<const uint8_t> in, gsl::span<uint8_t> out);

};  // namespace fc::primitives::piece
