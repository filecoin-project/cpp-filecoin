/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/piece/piece.hpp"

#include <cmath>
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
      return PieceError::kLessThatMinimumSize;
    }

    // is 127 * 2^n
    if ((size_ % 127) || ((size_ / 127) & ((size_ / 127) - 1))) {
      return PieceError::kInvalidUnpaddedSize;
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

  PaddedPieceSize &PaddedPieceSize::operator+=(uint64_t rhs) {
    size_ += rhs;
    return *this;
  }

  UnpaddedPieceSize PaddedPieceSize::unpadded() const {
    return UnpaddedPieceSize(size_ - (size_ / 128));
  }

  outcome::result<void> PaddedPieceSize::validate() const {
    if (size_ < 128) {
      return PieceError::kLessThatMinimumPaddedSize;
    }

    if ((size_) & (size_ - 1)) {
      return PieceError::kInvalidPaddedSize;
    }

    return outcome::success();
  }

  UnpaddedPieceSize paddedSize(uint64_t size) {
    int logv = static_cast<int>(floor(std::log2(size))) + 1;

    uint64_t sect_size = (1 << logv);
    UnpaddedPieceSize bound = PaddedPieceSize(sect_size).unpadded();
    if (size <= bound) {
      return bound;
    }

    return PaddedPieceSize(1 << (logv + 1)).unpadded();
  }

  PaddedByteIndex paddedIndex(UnpaddedByteIndex index) {
    return PaddedByteIndex(UnpaddedPieceSize(index).padded());
  }
  UnpaddedByteIndex unpaddedIndex(PaddedByteIndex index) {
    return UnpaddedByteIndex(PaddedPieceSize(index).unpadded());
  }

  void innerUnpad(gsl::span<const uint8_t> in, gsl::span<uint8_t> out) {
    auto chunks = in.size() / 128;
    for (auto chunk = 0; chunk < chunks; chunk++) {
      auto input_offset_next = chunk * 128 + 1;
      auto output_offset = chunk * 127;

      auto current = in[chunk * 128];

      for (size_t i = 0; i < 32; i++) {
        out[output_offset + i] = current;

        current = in[i + input_offset_next];
      }

      out[output_offset + 31] |= current << 6;

      for (size_t i = 32; i < 64; i++) {
        auto next = in[i + input_offset_next];

        out[output_offset + i] = current >> 2;
        out[output_offset + i] |= next << 6;

        current = next;
      }

      out[output_offset + 63] ^= (current << 6) ^ (current << 4);

      for (size_t i = 64; i < 96; i++) {
        auto next = in[i + input_offset_next];

        out[output_offset + i] = current >> 4;
        out[output_offset + i] |= next << 4;

        current = next;
      }

      out[output_offset + 95] ^= (current << 4) ^ (current << 2);

      for (size_t i = 96; i < 127; i++) {
        auto next = in[i + input_offset_next];

        out[output_offset + i] = current >> 6;
        out[output_offset + i] |= next << 2;

        current = next;
      }
    }
  }

  void innerPad(gsl::span<const uint8_t> in, gsl::span<uint8_t> out) {
    auto chunks = out.size() / 128;
    for (auto chunk = 0; chunk < chunks; chunk++) {
      size_t input_offset = chunk * 127;
      size_t output_offset = chunk * 128;

      std::copy(in.begin() + input_offset,
                in.begin() + input_offset + 31,
                out.begin() + output_offset);

      auto t = in[input_offset + 31] >> 6;
      out[output_offset + 31] = in[input_offset + 31] & 0x3f;
      uint8_t v;

      for (int i = 32; i < 64; i++) {
        v = in[input_offset + i];
        out[output_offset + i] = (v << 2) | t;
        t = v >> 6;
      }

      t = v >> 4;
      out[output_offset + 63] &= 0x3f;

      for (size_t i = 64; i < 96; i++) {
        v = in[input_offset + i];
        out[output_offset + i] = (v << 4) | t;
        t = v >> 4;
      }

      t = v >> 2;
      out[output_offset + 95] &= 0x3f;

      for (size_t i = 96; i < 127; i++) {
        v = in[input_offset + i];
        out[output_offset + i] = (v << 6) | t;
        t = v >> 2;
      }

      out[output_offset + 127] = t & 0x3f;
    }
  }

  void pad(gsl::span<const uint8_t> in, gsl::span<uint8_t> out) {
    if (uint64_t(out.size()) > kMultithreadingTreshold) {
      // TODO: multithreading option
    }

    innerPad(in, out);
  }

  void unpad(gsl::span<const uint8_t> in, gsl::span<uint8_t> out) {
    if (uint64_t(in.size()) > kMultithreadingTreshold) {
      // TODO: multithreading option
    }

    innerUnpad(in, out);
  }
}  // namespace fc::primitives::piece
