/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_CODECS_LEB128DECODESTREAM_HPP
#define FILECOIN_CORE_CODECS_LEB128DECODESTREAM_HPP

#include <stdexcept>
#include <vector>

#include "primitives/boost_multiprecision.hpp"

namespace fc::codec::leb128 {
  /**
   * LEB128 decode stream
   */
  class LEB128DecodeStream {
   public:
    static constexpr auto is_decoder_stream = true;

    /**
     * @brief Decode in byte-by-byte mode
     * @param byte - next byte to decode
     * @return decoded stream
     */
    LEB128DecodeStream(const std::vector<uint8_t> &input) {
      data_ = input;
    }

    /**
     * @brief LEB-128 decode data
     * @tparam T - type of decoded data
     * @param output - decoded data
     * @return Decoded stream
     * @throw std::ivalid_argument if encoded data is too big
     */
    template <typename T>
    LEB128DecodeStream &operator>>(T &output) {
      output = 0;
      size_t index = 0;
      constexpr size_t capacity = sizeof(T) * 8;
      for (const auto &byte : data_) {
        T slice = byte & 0x7f;
        if (index >= capacity || slice << index >> index != slice) {
          throw std::invalid_argument("Output value overflow");
        }
        output += static_cast<T>((byte & 0x7f)) << index;
        index += 7;
      }
      return *this;
    }

   private:
    std::vector<uint8_t> data_; /**< Stream content */
  };
};  // namespace fc::codec::leb128

#endif  // FILECOIN_CORE_CODECS_LEB128DECODESTREAM_HPP
