/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_RLE_PLUS_ENCODING_STREAM_HPP
#define CPP_FILECOIN_RLE_PLUS_ENCODING_STREAM_HPP

#include <vector>
#include <set>

#include <boost/dynamic_bitset.hpp>

#include "codec/rle/rle_plus_config.hpp"

namespace fc::codec::rle {
  /**
   * @class RLE+ encoding stream
   */
  class RLEPlusEncodingStream {
   public:
    /**
     * @brief Encode integer set
     * @tparam T - type of integer
     * @tparam A - set's allocator
     * @param input - content of the set to encode
     * @return Encoded stream
     */
    template <typename T, typename A>
    RLEPlusEncodingStream &operator<<(const std::set<T, A> &input) {
      std::vector<T> periods = this->getPeriods(input);
      this->initContent();
      bool flag = false;
      if (!input.empty()) flag = *input.begin() == 0;
      content_.push_back(flag);
      for (const auto &value : periods) {
        if (value == 1) {
          content_.push_back(true);
        } else if (value < LONG_BLOCK_VALUE) {
          this->pushSmallBlock(value);
        } else if (value >= LONG_BLOCK_VALUE) {
          this->pushLongBlock(value);
        }
      }
      return *this;
    }

    /**
     * @brief Get encoded stream content
     * @return Stream content
     */
    std::vector<uint8_t> data() const {
      std::vector<uint8_t> data_content;
      data_content.reserve(content_.num_blocks());
      boost::to_block_range(content_, std::back_inserter(data_content));
      return data_content;
    }

   private:
    boost::dynamic_bitset<uint8_t> content_{}; /**< RLE+ encoded content */

    /**
     * @brief Write RLE+ header
     */
    void initContent();

    /**
     * @brief Write RLE+ small block
     * @tparam T - type of block value
     * @param block - value to write
     */
    template <typename T>
    void pushSmallBlock(const T block) {
      content_.push_back(false);
      content_.push_back(true);
      for (size_t i = 0; i < SMALL_BLOCK_LENGTH; ++i) {
        content_.push_back((block & (1 << i)) >> i);
      }
    }

    /**
     * @brief Write RLE+ long block
     * @tparam T - type of block value
     * @param block - value to write
     */
    template <typename T>
    void pushLongBlock(const T block) {
      uint8_t byte{};
      T slice = block;
      content_.push_back(false);
      content_.push_back(false);
      while (slice >= BYTE_SLICE_VALUE) {
        byte = slice | BYTE_SLICE_VALUE;
        this->pushByte(byte);
        slice >>= PACK_BYTE_SHIFT;
      }
      this->pushByte(static_cast<uint8_t>(slice));
    }

    /**
     * @brief Write various byte
     * @param byte - content to write
     */
    void pushByte(uint8_t byte);

    /**
     * @brief RLE+ logic - calculate periods of the integers
     * @tparam T - type of the integers
     * @tparam A - set's allocator
     * @param data - input set of the integers
     * @return Periods length
     */
    template <typename T, typename A>
    std::vector<T> getPeriods(const std::set<T, A> &data) const {
      std::vector<T> periods{};
      T prev = !data.empty() ? *data.begin() : 0;
      if (prev != 0) periods.push_back(prev);
      if (data.empty()) return periods;
      periods.push_back(1);
      for (auto index = ++data.begin(); index != data.end(); ++index) {
        T diff = *index - prev;
        if (diff == 1) periods.back()++;
        if (diff > 1) {
          periods.push_back(diff - 1);
          periods.push_back(1);
        }
        prev = *index;
      }
      return periods;
    }
  };
};  // namespace fc::codec::rle

#endif
