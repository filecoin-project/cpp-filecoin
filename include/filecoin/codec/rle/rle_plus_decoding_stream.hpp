/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_RLE_PLUS_DECODING_STREAM_HPP
#define CPP_FILECOIN_RLE_PLUS_DECODING_STREAM_HPP

#include <vector>
#include <set>

#include <boost/dynamic_bitset.hpp>
#include <gsl/span>

#include "filecoin/codec/rle/rle_plus_config.hpp"
#include "filecoin/codec/rle/rle_plus_errors.hpp"

namespace fc::codec::rle {
  /**
   * @class Decode RLE+ byte stream
   */
  class RLEPlusDecodingStream {
   public:
    /**
     * @brief Constructor
     * @param data - RLE+ encoded bytes
     */
    explicit RLEPlusDecodingStream(gsl::span<const uint8_t> data)
        : index_{}, magnitude_{}, content_{data.begin(), data.end()} {}

    /**
     * @brief Decode RLE+
     * @tparam T - type of output data
     * @param output - decoded data
     * @return Decoded stream
     */
    template <typename T>
    RLEPlusDecodingStream &operator>>(std::set<T> &output) {
      if ((content_.size() < SMALL_BLOCK_LENGTH)
          || (getSpan<uint8_t>(2) != 0)) {
        throw errors::VersionMismatch();
      }
      magnitude_ = getSpan<uint8_t>(1) == 1;
      T value = 0;
      while (content_.find_next(index_ - 1)
             != boost::dynamic_bitset<uint8_t>::npos) {
        auto header = getSpan<uint8_t>(1);
        if (header == 1) {
          decodeSingleBlock(value, output);
        } else if (header == 0) {
          auto block_header = getSpan<uint8_t>(1);
          if (block_header == 0) {
            decodeLongBlock(value, output);
          } else {
            decodeSmallBlock(value, output);
          }
        }
      }
      constexpr size_t max_size = OBJECT_MAX_SIZE / sizeof(T);
      if (output.size() > max_size) {
        throw errors::MaxSizeExceed();
      }
      return *this;
    }

   private:
    size_t index_;   /**< Content's current index */
    bool magnitude_; /**< Polarity of the current index */
    boost::dynamic_bitset<uint8_t> content_; /**< Encoded data */

    /**
     * @brief Get bit span, represented as T
     * @tparam T - type of value to return
     * @param count - number of bits to use
     * @return Bits [index_ : index_ + count], represented as T
     */
    template <typename T>
    T getSpan(size_t count) {
      T value{};
      size_t shift = 0;
      const size_t end = index_ + count;
      if (content_.size() < end) {
        throw errors::IndexOutOfBound();
      }
      for (size_t i = index_; i < end; ++i) {
        T slice = content_.test(i) ? 1 : 0;
        value |= slice << shift;
        ++shift;
      }
      index_ += count;
      return value;
    }

    /**
     * @brief Unpack bytes to T
     * @tparam T - type of the value to return
     * @param data - packed bytes
     * @return Unpacked data
     */
    template <typename T>
    T unpack(const std::vector<uint8_t> &data) {
      T value{};
      size_t shift{};
      const size_t max_shift = sizeof(T) * BYTE_BITS_COUNT;
      for (const auto &byte : data) {
        if (shift > max_shift) {
          throw errors::UnpackBytesOverflow{};
        }
        if (byte < BYTE_SLICE_VALUE) {
          value = value | (static_cast<T>(byte) << shift);
          break;
        }
        value |= static_cast<T>(byte & UNPACK_BYTE_MASK) << shift;
        shift += PACK_BYTE_SHIFT;
      }
      return value;
    }

    /**
     * @brief Decode single RLE+ block
     * @tparam T - type of the data to output
     * @param current_value - index of the current number
     * @param output - decoded data
     */
    template <typename T>
    void decodeSingleBlock(T &current_value, std::set<T> &output) {
      if (magnitude_) {
        output.insert(current_value);
      }
      magnitude_ = !magnitude_;
      current_value++;
    }

    /**
     * @brief Decode small RLE+ block
     * @tparam T - type of the data to output
     * @param current_value - index of the current number
     * @param output - decoded data
     */
    template <typename T>
    void decodeSmallBlock(T &current_value, std::set<T> &output) {
      size_t length = getSpan<uint8_t>(SMALL_BLOCK_LENGTH);
      if (magnitude_) {
        for (size_t i = 0; i < length; ++i) {
          output.insert(current_value++);
        }
      } else {
        current_value += length;
      }
      magnitude_ = !magnitude_;
    }

    /**
     * @brief Decode long RLE+ block
     * @tparam T - type of the data to output
     * @param current_value - index of the current number
     * @param output - decoded data
     */
    template <typename T>
    void decodeLongBlock(T &current_value, std::set<T> &output) {
      std::vector<uint8_t> bytes{};
      uint8_t slice;
      do {
        slice = getSpan<uint8_t>(BYTE_BITS_COUNT);
        bytes.push_back(slice);
      } while ((slice & BYTE_SLICE_VALUE) != 0);
      T length = unpack<T>(bytes);
      if (magnitude_) {
        for (size_t i = 0; i < length; ++i) {
          output.insert(current_value++);
        }
      } else {
        current_value += length;
      }
      magnitude_ = !magnitude_;
    }
  };
};  // namespace fc::codec::rle

#endif
