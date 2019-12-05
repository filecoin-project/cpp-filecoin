/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/codec/leb128/leb128_codec_tester.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

using boost::math::tools::max_value;
using boost::math::tools::min_value;
using boost::multiprecision::uint1024_t;
using boost::multiprecision::uint128_t;
using boost::multiprecision::uint256_t;
using boost::multiprecision::uint512_t;
using fc::codec::leb128::LEB128DecodeError;

/**
 * Support only unsigned values
 */
using types = testing::Types<uint8_t,
                             uint16_t,
                             uint32_t,
                             uint64_t,
                             uint128_t,
                             uint256_t,
                             uint512_t,
                             uint1024_t>;

TYPED_TEST_CASE(LEB128CodecTester,
                types); /**< Initialize tests suite with possible cases */

/**
 * @given Max and min values of the supported numeric types
 * @when Encoding/decoding value with LEB128 codec
 * @then Decoded values must be the same as source
 */
TYPED_TEST(LEB128CodecTester, BoundariesReversibilitySuccess) {
  ASSERT_TRUE(this->checkReversibility(min_value<TypeParam>()));
  ASSERT_TRUE(this->checkReversibility(max_value<TypeParam>()));
}

/**
 * @given LEB128 encoded samples
 * @when Encoding values with LEB128 codec
 * @then Encoded values must be the same as the samples
 */
TYPED_TEST(LEB128CodecTester, AlgorithmSuccess) {
  std::vector<uint8_t> encoded = fc::codec::leb128::encode(this->sample_.first);
  ASSERT_EQ(encoded, this->sample_.second);
}

/**
 * @given Empty byte-vector to be decoded
 * @when Decoding value with LEB128 codec
 * @then Attempt to decode empty value must be failed
 */
TYPED_TEST(LEB128CodecTester, DecodeEmptyVectorFailure) {
  std::vector<uint8_t> empty{};
  int expected =
      static_cast<int>(fc::codec::leb128::LEB128DecodeError::INPUT_EMPTY);
  ASSERT_TRUE(this->checkDecodeFail(
      empty, expected));
}

/**
 * @given LEB128-encoded value (2^64), next after max value of the uint64_t (2^64 - 1)
 * @when Decoding value with LEB128 codec
 * @then Attempt to decode too big value must be failed
 */
TEST(LEB128CodecTest, DecodeSampleOverflowFailure) {
  std::vector<uint8_t> encoded{0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02};
  int expected =
      static_cast<int>(fc::codec::leb128::LEB128DecodeError::INPUT_TOO_BIG);
  EXPECT_OUTCOME_FALSE_3(result, error_code, fc::codec::leb128::decode<uint64_t>(encoded));
  ASSERT_EQ(expected, error_code.value());
}
