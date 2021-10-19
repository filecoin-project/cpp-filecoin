/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/endian/buffers.hpp>
#include "common/bytes.hpp"

namespace fc::common {
  /**
   * @brief little-endian-encodes an integral value
   * @tparam T integral value type
   * @param value integral value to encode
   * @param out output buffer
   */
  template <class T,
            typename I = std::decay_t<T>,
            typename = std::enable_if_t<std::is_integral<I>::value>>
  void encodeLebInteger(T value,
                        Bytes &out) {  // no need to take integers by &&
    constexpr size_t size = sizeof(T);
    constexpr size_t bits = size * 8;
    boost::endian::endian_buffer<boost::endian::order::little, T, bits> buf{};
    buf = static_cast<I>(value);  // cannot initialize, only assign
    for (size_t i = 0; i < size; ++i) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      out.push_back(static_cast<uint8_t>(buf.data()[i]));
    }
  }
}  // namespace fc::common
