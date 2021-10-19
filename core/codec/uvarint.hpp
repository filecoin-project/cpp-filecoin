/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <gsl/span>
#include <limits>

#include "codec/common.hpp"

#include "common/file.hpp"

namespace fc::codec::uvarint {
  struct VarintDecoder {
    size_t max_bits{64};
    uint64_t value{0};
    bool more{true};
    bool overflow{false};
    size_t length{0};

    inline void update(uint8_t byte) {
      assert(more);
      assert(!overflow);
      more = (byte & 0x80) != 0;
      auto bits7{byte & 0x7f};
      auto shift{length * 7};
      auto more_bits{max_bits - shift};
      overflow = more_bits < 7 && ((bits7 & (-1 << more_bits)) != 0);
      value |= bits7 << shift;
      ++length;
    }
  };

  struct VarintEncoder {
    uint64_t value;
    BytesN<10> _bytes{};
    size_t length{};

    constexpr explicit VarintEncoder(uint64_t _value) : value{_value} {
      do {
        auto byte{static_cast<uint8_t>(_value) & 0x7f};
        _value >>= 7;
        if (_value != 0) {
          byte |= 0x80;
        }
        _bytes[length] = byte;
        ++length;
      } while (_value != 0);
    }
    constexpr auto bytes() const {
      return gsl::make_span(_bytes).subspan(0, static_cast<ptrdiff_t>(length));
    }
  };

  /** returns true on success */
  inline bool read(std::istream &is, VarintDecoder &varint) {
    while (varint.more) {
      char byte = 0;
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
                          Bytes &buffer,
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

  template <typename T>
  inline bool read(T &out, BytesIn &input) {
    out = {};
    VarintDecoder varint;
    if constexpr (std::is_enum_v<T>) {
      varint.max_bits = std::numeric_limits<std::underlying_type_t<T>>::digits;
    } else {
      varint.max_bits = std::numeric_limits<T>::digits;
    }
    for (auto byte : input) {
      varint.update(byte);
      if (varint.overflow) {
        return false;
      }
      if (!varint.more) {
        input = input.subspan(static_cast<ptrdiff_t>(varint.length));
        out = static_cast<T>(varint.value);
        return true;
      }
    }
    return false;
  }

  inline bool readBytes(BytesIn &out, BytesIn &input) {
    out = {};
    size_t length = 0;
    return read(length, input) && fc::codec::read(out, input, length);
  }
}  // namespace fc::codec::uvarint
