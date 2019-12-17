/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "core/codec/rleplus/rle_plus_codec_tester.hpp"

using fc::codec::rle::decode;
using fc::codec::rle::RLEPlusDecodeError;
using fc::codec::rle::encode;

using types = testing::Types<uint8_t, uint16_t, uint32_t, uint64_t>;

TYPED_TEST_CASE(RLEPlusCodecTester, types);

/**
 * @given Null set of data
 * @when RLE+ encode and following decode back
 * @then Operations must be completed successfully and decoded data must be the
 * same as the given set
 */
TYPED_TEST(RLEPlusCodecTester, NullDataSuccess) {
  this->checkDataSet({});
}

/**
 * @given Data set with single content
 * @when RLE+ encode and following decode back
 * @then Operations must be completed successfully and decoded data must be the
 * same as the given set
 */
TYPED_TEST(RLEPlusCodecTester, ZeroDataSuccess) {
  this->checkDataSet({0});
}

/**
 * @given Data set with single content
 * @when RLE+ encode and following decode back
 * @then Operations must be completed successfully and decoded data must be the
 * same as the given set
 */
TYPED_TEST(RLEPlusCodecTester, OneDataSuccess) {
  this->checkDataSet({1});
}

/**
 * @given Set of single blocks from T::min to T::max
 * @when RLE+ encode and following decode back
 * @then Operations must be completed successfully and decoded data must be the
 * same as the given set
 */
TYPED_TEST(RLEPlusCodecTester, SingleBlocksFromZeroSuccess) {
  auto &&data_set =
      this->generateDataSet(std::numeric_limits<TypeParam>::min(),
                            std::numeric_limits<TypeParam>::max(),
                            [](TypeParam prev) { return prev += 2; });
  this->checkDataSet(data_set);
}

/**
 * @given Set of single blocks from T::max / 2 to T::max
 * @when RLE+ encode and following decode back
 * @then Operations must be completed successfully and decoded data must be the
 * same as the given set
 */
TYPED_TEST(RLEPlusCodecTester, SignleBlocksFromMiddleSuccess) {
  auto &&data_set =
      this->generateDataSet(std::numeric_limits<TypeParam>::max() / 2,
                            std::numeric_limits<TypeParam>::max(),
                            [](TypeParam prev) { return prev += 2; });
  this->checkDataSet(data_set);
}

/**
 * @given Set of mixed type blocks
 * @when RLE+ encode and following decode back
 * @then Operations must be completed successfully and decoded data must be the
 * same as the given set
 */
TYPED_TEST(RLEPlusCodecTester, MixedBlocksSuccess) {
  auto &&data_set = this->generateDataSet(
      std::numeric_limits<TypeParam>::min(),
      std::numeric_limits<TypeParam>::max() / static_cast<TypeParam>(5),
      [](TypeParam prev) -> TypeParam { return prev += (prev % 100) + 1; });
  this->checkDataSet(data_set);
}

/**
 * @given RLE+ encoded data with invalid header version
 * @when RLE+ decode given data
 * @then Decode operation must be failed with appropriate error code
 */
TYPED_TEST(RLEPlusCodecTester, InvalidHeaderDecodeFailure) {
  auto expected = RLEPlusDecodeError::VersionMismatch;
  std::vector<uint8_t> data{0xFF, 0x8, 0x15, 0x16};
  this->checkDecodeFailure(data, expected);
}

/**
 * @given RLE+ encoded data with invalid structure
 * @when RLE+ decode given data
 * @then Decode operation must be failed with appropriate error code
 */
TYPED_TEST(RLEPlusCodecTester, InvalidStructureDecodeFailure) {
  auto expected = RLEPlusDecodeError::DataIndexFailure;
  std::vector<uint8_t> data{0x4, 0x8, 0x15, 0x16};
  this->checkDecodeFailure(data, expected);
}

/**
 * @given Reference RLE+ encoded and decoded data
 * @when RLE+ encode given data
 * @tparam Decode operation must be successful and decoded data must be the same
 * as the given
 */
TYPED_TEST(RLEPlusCodecTester, ReferenceComparingSuccess) {
  auto encoded = encode(this->reference_decoded_sample_);
  EXPECT_OUTCOME_TRUE(decoded, decode<TypeParam>(encoded));
  ASSERT_EQ(decoded, this->reference_decoded_sample_);
}

/**
 * @given RLE+ encoded data, which exceeds max object size
 * @when RLE+ decode given data
 * @then Decode operation must be failed with appropriate error code
 */
TEST(RLEPlusDecode, MaxSizeExceedFailure) {
  std::set<uint64_t> data_set;
  constexpr size_t max_length =
      fc::codec::rle::OBJECT_MAX_SIZE / sizeof(uint64_t) + 1;
  uint64_t value = 0;
  for (size_t i = 0; i < max_length; ++i) {
    data_set.emplace(value);
    value += 2;
  }
  auto encoded = encode(data_set);
  auto expected = RLEPlusDecodeError::MaxSizeExceed;
  auto result = decode<uint64_t>(encoded);
  ASSERT_TRUE(result.has_error());
  ASSERT_EQ(result.error().value(), static_cast<int>(expected));
}
