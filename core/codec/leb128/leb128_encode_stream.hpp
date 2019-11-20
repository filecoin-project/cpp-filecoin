/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_CODECS_LEB128_ENCODE_STREAM_HPP
#define CORE_CODECS_LEB128_ENCODE_STREAM_HPP

#include "primitives/boost_multiprecision.hpp"

namespace fc::codec::leb128 {
  /**
   * @class LEB128 encoding stream
   * @tparam T - type of the value to encode
   */
  class LEB128EncodeStream {
   public:
    static constexpr auto is_encoder_stream = true;
    /**
     * @brief Encode data to LEB128
     * @param data - value to encode
     * @return encoded stream
     */
    template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
    LEB128EncodeStream &operator<<(T data) {
      do {
        uint8_t byte = static_cast<uint8_t>(data) & 0x7f;
        data >>= 7;
        if (data != 0) byte |= 0x80;
        this->content_.push_back(byte);
      } while (data != 0);
      return *this;
    }

    /**
     * @brief Get encoded content
     * @return Byte-vector with encoded value
     */
    const std::vector<uint8_t> &data() const {
      return content_;
    }

   protected:
    std::vector<uint8_t> content_{}; /**< Stream content */
  };
};  // namespace fc::codec::leb128

#endif  // CORE_CODECS_LEB128_ENCODE_STREAM_HPP
