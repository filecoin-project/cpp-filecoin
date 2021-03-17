/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gsl/span>
#include <limits>

#include "common/file.hpp"
#include "common/outcome.hpp"

namespace fc::codec::uvarint {
  using Input = gsl::span<const uint8_t>;

  struct VarintDecoder {
    size_t max_bits{64};
    uint64_t value{0};
    bool more{true};
    bool overflow{false};
    size_t length{0};

    inline void update(uint8_t byte) {
      assert(more);
      assert(!overflow);
      more = byte & 0x80;
      auto bits7{byte & 0x7f};
      auto shift{length * 7};
      auto more_bits{max_bits - shift};
      overflow = more_bits < 7 && (bits7 & (-1 << more_bits));
      value |= bits7 << shift;
      shift += 7;
      ++length;
    }
  };

  /** returns true on success */
  inline bool read(std::istream &is, VarintDecoder &varint) {
    while (varint.more) {
      char byte;
      if (!is.get(byte).good()) {
        return false;
      }
      varint.update(byte);
      if (varint.overflow) {
        return false;
      }
    }
    return true;
  }

  /** returns varint length (always > 0) on success else 0 */
  inline size_t readBytes(std::istream &is,
                          Buffer &buffer,
                          size_t max = 1 << 30) {
    VarintDecoder varint;
    if (read(is, varint) && varint.value <= max) {
      buffer.resize(varint.value);
      if (common::read(is, gsl::make_span(buffer))) {
        return varint.length;
      }
    }
    return 0;
  }

  template <auto error, typename T = uint64_t>
  outcome::result<T> read(Input &input) {
    VarintDecoder varint;
    if constexpr (std::is_enum_v<T>) {
      varint.max_bits = std::numeric_limits<std::underlying_type_t<T>>::digits;
    } else {
      varint.max_bits = std::numeric_limits<T>::digits;
    }
    for (auto byte : input) {
      varint.update(byte);
      if (varint.overflow) {
        return error;
      }
      if (!varint.more) {
        input = input.subspan(varint.length);
        return static_cast<T>(varint.value);
      }
    }
    return error;
  }

  template <auto error_length, auto error_data>
  outcome::result<Input> readBytes(Input &input) {
    OUTCOME_TRY(size, read<error_length>(input));
    if (input.size() < static_cast<ptrdiff_t>(size)) {
      return error_data;
    }
    auto result = input.subspan(0, size);
    input = input.subspan(size);
    return result;
  }
}  // namespace fc::codec::uvarint
