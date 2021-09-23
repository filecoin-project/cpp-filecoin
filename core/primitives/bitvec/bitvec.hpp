/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>

namespace fc::primitives::bitvec {

  class BitvecReader {
   public:
    explicit BitvecReader(const std::vector<uint8_t> &buf) : buffer(buf) {
      if (not buf.empty()) {
        bits = uint16_t(buffer[0]);
      }
    }

    /**
     * Get byte from buffer with shift
     * @return byte from buffer
     */
    uint8_t getByte() {
      uint8_t res(bits);

      bits >>= 8;

      if (index < buffer.size()) {
        bits |= uint16_t(buffer[index]) << (bitsCap - 8);
      }

      ++index;
      return res;
    }

    /**
     * Get 6 bit from buffer without shift
     * @return byte from buffer
     */
    uint8_t peek6Bit() const {
      return uint8_t(bits) & 0x3f;
    }

    /**
     * Get 1 bit from buffer with shift
     * @return bit from buffer
     */
    bool getBit() {
      bool res = (bits & 0x1) != 0;
      bits >>= 1;
      --bitsCap;

      if (index < buffer.size()) {
        bits |= uint16_t(buffer[index]) << bitsCap;
      }

      if (bitsCap < 8) {
        index += 1;
        bitsCap += 8;
      }

      return res;
    }

    /**
     * get @count bits from buffer
     * @param count this is the number of bits to get from buffer
     * @note if @count greater than 8 then @count - 8 bits will be disappear
     * @return 1 byte
     */
    uint8_t get(uint8_t count) {
      auto f = uint8_t(bits);
      auto b = ((1 << count) - 1);
      uint8_t res = f & b;
      bits >>= count;
      bitsCap -= count;

      if (index < buffer.size()) {
        bits |= uint16_t(buffer[index]) << bitsCap;
      }

      if (bitsCap < 8) {
        index += 1;
        bitsCap += 8;
      }

      return res;
    }

   private:
    uint64_t index = 1;

    std::vector<uint8_t> buffer;
    uint16_t bits = 0;
    uint8_t bitsCap = 8;
  };

  class BitvecWriter {
   public:
    BitvecWriter() = default;

    /**
     * Flush content to vector
     * @return bitvec vector
     */
    std::vector<uint8_t> out() {
      if (bitsCap != 0) {
        buffer.push_back(uint8_t(bits));
      }
      if (bitsCap > 8) {
        buffer.push_back(uint8_t(bits >> 8));
      }
      bitsCap = 0;
      bits = 0;

      if (buffer.empty()) {
        return buffer;
      }

      uint64_t end = buffer.size();
      while ((end != 0) && buffer[end - 1] == 0) {
        --end;
      }
      buffer.resize(end);
      return buffer;
    }

    /**
     * Put byte to bitvec
     * @param val is byte
     * @param count is number of bits it takes to keep @val
     */
    void put(uint8_t val, uint8_t count) {
      bits |= uint16_t(val) << bitsCap;
      bitsCap += count;

      if (bitsCap >= 8) {
        buffer.push_back(uint8_t(bits));
        bitsCap -= 8;
        bits >>= 8;
      }
    }

   private:
    std::vector<uint8_t> buffer;
    uint16_t bits = 0;
    uint8_t bitsCap = 0;
  };

}  // namespace fc::primitives::bitvec
