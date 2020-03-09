/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CODEC_RLE_PLUS_HPP
#define CODEC_RLE_PLUS_HPP

#include "filecoin/common/outcome.hpp"
#include "filecoin/codec/rle/rle_plus_errors.hpp"
#include "filecoin/codec/rle/rle_plus_decoding_stream.hpp"
#include "filecoin/codec/rle/rle_plus_encoding_stream.hpp"

namespace fc::codec::rle {
  /**
   * @brief RLE+ encode
   * @tparam T - type of elements to encode
   * @tparam A - std::set allocator
   * @param input - data to encode
   * @return Encoded byte-vector
   */
  template <typename T, typename A>
  std::vector<uint8_t> encode(const std::set<T, A> &input) {
    RLEPlusEncodingStream encoder;
    encoder << input;
    return encoder.data();
  }

  /**
   * @brief RLE+ decode
   * @tparam T - type of elements to decode
   * @param input - data to decode
   * @return Decoded data
   */
  template <typename T>
  outcome::result<std::set<T>> decode(gsl::span<const uint8_t> input) {
    std::set<T> data;
    RLEPlusDecodingStream decoder(input);
    try {
      decoder >> data;
    } catch (errors::VersionMismatch&) {
      return RLEPlusDecodeError::VersionMismatch;
    } catch (errors::IndexOutOfBound&) {
      return RLEPlusDecodeError::DataIndexFailure;
    } catch (errors::UnpackBytesOverflow&) {
      return RLEPlusDecodeError::UnpackOverflow;
    } catch (errors::MaxSizeExceed&) {
      return RLEPlusDecodeError::MaxSizeExceed;
    }
    return data;
  }
};  // namespace fc::codec::rle

#endif
