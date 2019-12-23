/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_HPP

#include "codec/cbor/cbor_decode_stream.hpp"
#include "codec/cbor/cbor_encode_stream.hpp"
#include "codec/cbor/cbor_resolve.hpp"

namespace fc::codec::cbor {
  /**
   * @brief CBOR encoding to byte-vector
   * @tparam Type to be encoded
   * @param arg data to be encoded
   * @return encoded data
   */
  template <typename T>
  outcome::result<std::vector<uint8_t>> encode(const T &arg) {
    try {
      CborEncodeStream encoder;
      encoder << arg;
      return encoder.data();
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }
  }

  /**
   * @brief CBOR decoding from byte-vector
   * @tparam T - type of the value to decode
   * @param input - data to decode
   * @return operation result
   * @see cbor_errors.hpp for possible error cases
   */
  template <typename T>
  outcome::result<T> decode(gsl::span<const uint8_t> input) {
    try {
      T data;
      CborDecodeStream decoder(input);
      decoder >> data;
      return data;
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }
  }
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_HPP
