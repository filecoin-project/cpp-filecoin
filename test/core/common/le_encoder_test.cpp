/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/common/le_encoder.hpp"

#include <gtest/gtest.h>

using fc::common::Buffer;
using fc::common::encodeLebInteger;

template <typename T>
class IntegerTest : public ::testing::TestWithParam<std::pair<T, Buffer>> {
 public:
  static std::pair<T, Buffer> make_pair(T value, const Buffer &match) {
    return std::make_pair(value, match);
  }

  using value_type = T;
};

/**
 * @brief class for testing int8_t encode and decode
 */
class Int8Test : public IntegerTest<int8_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Int8Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  Buffer s{};
  encodeLebInteger(value, s);
  ASSERT_EQ(s, match);
}

INSTANTIATE_TEST_CASE_P(Int8TestCases,
                        Int8Test,
                        ::testing::Values(Int8Test::make_pair(0, {0}),
                                          Int8Test::make_pair(-1, {255}),
                                          Int8Test::make_pair(-128, {128}),
                                          Int8Test::make_pair(-127, {129}),
                                          Int8Test::make_pair(123, {123}),
                                          Int8Test::make_pair(-15, {241})));

/**
 * @brief class for testing uint8_t encode and decode
 */
class Uint8Test : public IntegerTest<uint8_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Uint8Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  Buffer s{};
  encodeLebInteger(value, s);
  ASSERT_EQ(s, match);
}

INSTANTIATE_TEST_CASE_P(Uint8TestCases,
                        Uint8Test,
                        ::testing::Values(Uint8Test::make_pair(0, {0}),
                                          Uint8Test::make_pair(234, {234}),
                                          Uint8Test::make_pair(255, {255})));

/**
 * @brief class for testing int16_t encode and decode
 */
class Int16Test : public IntegerTest<int16_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Int16Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  Buffer s{};
  encodeLebInteger(value, s);
  ASSERT_EQ(s, match);
}

INSTANTIATE_TEST_CASE_P(
    Int16TestCases,
    Int16Test,
    ::testing::Values(Int16Test::make_pair(-32767, {1, 128}),
                      Int16Test::make_pair(-32768, {0, 128}),
                      Int16Test::make_pair(-1, {255, 255}),
                      Int16Test::make_pair(32767, {255, 127}),
                      Int16Test::make_pair(12345, {57, 48}),
                      Int16Test::make_pair(-12345, {199, 207})));

/**
 * @brief class for testing uint16_t encode and decode
 */
class Uint16Test : public IntegerTest<uint16_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Uint16Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  Buffer s{};
  encodeLebInteger(value, s);
  ASSERT_EQ(s, match);
}

INSTANTIATE_TEST_CASE_P(
    Uint16TestCases,
    Uint16Test,
    ::testing::Values(Uint16Test::make_pair(32767, {255, 127}),
                      Uint16Test::make_pair(12345, {57, 48})));

/**
 * @brief class for testing int32_t encode and decode
 */
class Int32Test : public IntegerTest<int32_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Int32Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  Buffer s{};
  encodeLebInteger(value, s);
  ASSERT_EQ(s, match);
}

INSTANTIATE_TEST_CASE_P(
    Int32TestCases,
    Int32Test,
    ::testing::Values(Int32Test::make_pair(2147483647l, {255, 255, 255, 127}),
                      Int32Test::make_pair(-1, {255, 255, 255, 255})));

/**
 * @brief class for testing uint32_t encode and decode
 */
class Uint32Test : public IntegerTest<uint32_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Uint32Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  Buffer s{};
  encodeLebInteger(value, s);
  ASSERT_EQ(s, match);
}

INSTANTIATE_TEST_CASE_P(
    Uint32TestCases,
    Uint32Test,
    ::testing::Values(Uint32Test::make_pair(16909060ul, {4, 3, 2, 1}),
                      Uint32Test::make_pair(67305985, {1, 2, 3, 4})));

/**
 * @brief class for testing int64_t encode and decode
 */
class Int64Test : public IntegerTest<int64_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Int64Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  Buffer s{};
  encodeLebInteger(value, s);
  ASSERT_EQ(s, match);
}

INSTANTIATE_TEST_CASE_P(
    Int64TestCases,
    Int64Test,
    ::testing::Values(
        Int64Test::make_pair(578437695752307201ll, {1, 2, 3, 4, 5, 6, 7, 8}),
        Int64Test::make_pair(-1, {255, 255, 255, 255, 255, 255, 255, 255})));

/**
 * @brief class for testing uint64_t encode and decode
 */
class Uint64Test : public IntegerTest<uint64_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Uint64Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  Buffer s{};
  encodeLebInteger(value, s);
  ASSERT_EQ(s, match);
}

INSTANTIATE_TEST_CASE_P(Uint64TestCases,
                        Uint64Test,
                        ::testing::Values(Uint64Test::make_pair(
                            578437695752307201ull, {1, 2, 3, 4, 5, 6, 7, 8})));
