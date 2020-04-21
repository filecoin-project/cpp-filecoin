/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/piece/piece.hpp"

#include <utility>
#include "primitives/piece/piece_error.hpp"

namespace fc::primitives::piece {

  UnpaddedPieceSize::UnpaddedPieceSize() : size_{} {}

  UnpaddedPieceSize::UnpaddedPieceSize(uint64_t size) : size_(size) {}

  UnpaddedPieceSize::operator uint64_t() const {
    return size_;
  }

  UnpaddedPieceSize &UnpaddedPieceSize::operator=(uint64_t rhs) {
    size_ = rhs;
    return *this;
  }

  UnpaddedPieceSize &UnpaddedPieceSize::operator+=(uint64_t rhs) {
    size_ += rhs;
    return *this;
  }

  PaddedPieceSize UnpaddedPieceSize::padded() const {
    return PaddedPieceSize(size_ + (size_ / 127));
  }

  outcome::result<void> UnpaddedPieceSize::validate() const {
    if (size_ < 127) {
      return PieceError::LESS_THAT_MINIMUM_SIZE;
    }

    // is 127 * 2^n
    if (!((size_ % 127) || ((size_ / 127) & ((size_ / 127) - 1)))) {
      return PieceError::INVALID_UNPADDED_SIZE;
    }

    return outcome::success();
  }

  PaddedPieceSize::PaddedPieceSize() : size_{} {}

  PaddedPieceSize::PaddedPieceSize(uint64_t size) : size_(size) {}

  PaddedPieceSize::operator uint64_t() const {
    return size_;
  }

  PaddedPieceSize &PaddedPieceSize::operator=(uint64_t rhs) {
    size_ = rhs;
    return *this;
  }

  UnpaddedPieceSize PaddedPieceSize::unpadded() const {
    return UnpaddedPieceSize(size_ - (size_ / 128));
  }

  outcome::result<void> PaddedPieceSize::validate() const {
    if (size_ < 128) {
      return PieceError::LESS_THAT_MINIMUM_PADDED_SIZE;
    }

    if (!((size_) & (size_ - 1))) {
      return PieceError::INVALID_PADDED_SIZE;
    }

    return outcome::success();
  }

  PieceInfo::PieceInfo(const PaddedPieceSize &padded_size, CID piece_CID)
      : size(padded_size), cid(std::move(piece_CID)) {}
}  // namespace fc::primitives::piece
