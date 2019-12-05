/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_CODECS_LEB128_HPP
#define FILECOIN_CORE_CODECS_LEB128_HPP

#include "codec/leb128/leb128_decode_stream.hpp"
#include "codec/leb128/leb128_encode_stream.hpp"
#include "codec/leb128/leb128_errors.hpp"
#include "common/outcome.hpp"

/**
 * @brief Interface declaration
 */
namespace fc::codec::leb128 {

  /**
   * @brief LEB128 encoding to byte-vector
   * @tparam Primitive type to be encoded
   * @param arg data to be encoded
   * @return encoded data
   */
  template <typename T>
  std::vector<uint8_t> encode(const T &arg) {
    LEB128EncodeStream encoder;
    encoder << arg;
    return encoder.data();
  }

  /**
   * @brief LEB128 decoding from byte-vector
   * @tparam T - type of the value to decode
   * @param input - data to decode
   * @return operation result
   * @see leb128_errors.hpp for possible error cases
   */
  template <typename T>
  outcome::result<T> decode(const std::vector<uint8_t> &input) {
    T data;
    LEB128DecodeStream decoder(input);
    if (input.empty()) {
      return outcome::failure(LEB128DecodeError::INPUT_EMPTY);
    }
    try {
      decoder >> data;
    } catch (std::exception&) {
      return outcome::failure(LEB128DecodeError::INPUT_TOO_BIG);
    }
    return data;
  }

};  // namespace fc::codec::leb128

#endif  // FILECOIN_CORE_CODECS_LEB128_HPP
