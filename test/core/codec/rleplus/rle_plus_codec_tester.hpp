/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_CORE_CODEC_RLE_PLUS_CODEC_TESTER_HPP
#define TEST_CORE_CODEC_RLE_PLUS_CODEC_TESTER_HPP

#include <limits>

#include <gtest/gtest.h>

#include "filecoin/codec/rle/rle_plus.hpp"
#include "testutil/outcome.hpp"

/**
 * @class Test fixture for RLE+ unit tests
 * @tparam T - type of data for codec
 */
template <typename T>
class RLEPlusCodecTester : public ::testing::Test {
 protected:
  /**
   * @var Decoded sample, produced by reference go lang implementation
   */
  std::set<T> reference_decoded_sample_{0,  2,  4,  5,  6,  11, 12, 13,
                                        14, 15, 16, 17, 18, 19, 20, 21,
                                        22, 23, 24, 25, 26, 27};

  /**
   * @var Encoded sample, produced by reference go lang implementation
   */
  std::set<T> reference_encoded_sample_{0x7C, 0x47, 0x22, 0x2};

  /**
   * @brief Codec algorithm check
   * @param data_set - data for test
   */
  void checkDataSet(const std::set<T> &data_set) {
    auto encoded = fc::codec::rle::encode(data_set);
    EXPECT_OUTCOME_TRUE(decoded, fc::codec::rle::decode<T>(encoded))
    ASSERT_EQ(data_set, decoded);
  }

  /**
   * @brief Codec algorithm error handling check
   * @param data - data for test
   * @param e - expected error code
   */
  void checkDecodeFailure(const std::vector<uint8_t> &data,
                          fc::codec::rle::RLEPlusDecodeError e) {
    auto result = fc::codec::rle::decode<T>(data);
    ASSERT_TRUE(result.has_error());
    ASSERT_EQ(result.error().value(), static_cast<int>(e));
  }

  /**
   * @brief Generate sample data set with specific params
   * @param value - start value of the set
   * @param end - max data set value
   * @param next - lambda to generate next data set value
   * @return Generated data set
   */
  std::set<T> generateDataSet(T value, T end, T (*next)(T)) {
    std::set<T> data;
    const size_t max_length =
        fc::codec::rle::OBJECT_MAX_SIZE / sizeof(T);
    for (size_t i = 0; (i < max_length) && (value != end); ++i) {
      data.emplace(value);
      value = next(value);
    }
    return data;
  }
};

#endif
