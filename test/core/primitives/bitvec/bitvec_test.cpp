/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/bitvec/bitvec.hpp"

#include <gtest/gtest.h>

/**
 * @given empty buffer and read bitvec
 * @when try to do something
 * @then returns 0 always
 */
TEST(ReadBitvec, EmptyBuffer) {
  using fc::primitives::BitvecReader;
  std::vector<uint8_t> buffer = {};
  BitvecReader reader(buffer);
  ASSERT_EQ(reader.getByte(), 0);
  ASSERT_EQ(reader.peek6Bit(), 0);
  ASSERT_EQ(reader.get(8), 0);  // 8 bit = 1 byte
  ASSERT_FALSE(reader.getBit());
}

/**
 * @given buffer with 2 byte and read bitvec
 * @when try to read byte twice
 * @then returns 1st and 2nd byte
 */
TEST(ReadBitvec, getByte) {
  using fc::primitives::BitvecReader;
  uint8_t result_byte = 8;
  uint8_t second_byte = 4;
  std::vector<uint8_t> buffer = {result_byte, second_byte};
  BitvecReader reader(buffer);
  ASSERT_EQ(reader.getByte(), result_byte);
  ASSERT_EQ(reader.getByte(), second_byte);
}

/**
 * @given buffer with byte and read bitvec
 * @when try to peek 6 bits
 * @then returns 6 bit of first byte
 */
TEST(ReadBitvec, peek6) {
  using fc::primitives::BitvecReader;
  uint8_t byte = 255;
  uint8_t result = 63;
  std::vector<uint8_t> buffer = {byte};
  BitvecReader reader(buffer);
  ASSERT_EQ(reader.peek6Bit(), result);
}

/**
 * @given buffer with byte and read bitvec
 * @when try to get bit twice
 * @then returns 1st and 2nd bit of first byte
 */
TEST(ReadBitvec, getBit) {
  using fc::primitives::BitvecReader;
  uint8_t byte = 2;
  std::vector<uint8_t> buffer = {byte};
  BitvecReader reader(buffer);
  ASSERT_FALSE(reader.getBit());
  ASSERT_TRUE(reader.getBit());
}

/**
 * @given buffer with byte and read bitvec
 * @when try to get 3 bit and then 1 bit
 * @then returns first 3 bit and 4th bit
 */
TEST(ReadBitvec, get) {
  using fc::primitives::BitvecReader;
  uint8_t byte = 15;
  uint8_t first_res = 7;
  uint8_t second_res = 1;
  std::vector<uint8_t> buffer = {byte};
  BitvecReader reader(buffer);
  ASSERT_EQ(reader.get(3), first_res);
  ASSERT_EQ(reader.get(1), second_res);
}

/**
 * @note examples in little endian
 *
 * @given write bitvec
 * @when try to write 5 values(with overflow 1 byte)
 * @then return bitvec that equals result vector
 */
TEST(WriteBitvec, out) {
  using fc::primitives::BitvecWriter;
  std::vector<uint8_t> res = {58, 1};  // 00111010 1
  BitvecWriter writer;
  writer.put(0, 1);  // 0
  writer.put(1, 1);  // 1
  writer.put(2, 2);  // 10
  writer.put(3, 2);  // 11
  writer.put(4, 3);  // 100
  ASSERT_EQ(writer.out(), res);
}
