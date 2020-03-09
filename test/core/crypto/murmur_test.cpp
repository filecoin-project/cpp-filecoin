/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/crypto/murmur/murmur.hpp"

#include <gtest/gtest.h>
#include "filecoin/common/hexutil.hpp"

class MurmurTest
    : public ::testing::TestWithParam<std::pair<std::string, std::string>> {
public:
  static std::pair<std::string, std::string> pair(const std::string &bytes, const std::string &hash) {
    return std::make_pair(bytes, hash);
  }
};

/**
 * @given Input bytes with pregenerated hash
 * @when Hash
 * @then Equals to pregenerated hash
 */
TEST_P(MurmurTest, GoCompatibility) {
  const auto &[bytes, hash] = GetParam();
  auto actual_array = fc::crypto::murmur::hash(fc::common::unhex(bytes).value());
  std::vector<uint8_t> actual_vector(actual_array.begin(), actual_array.end());
  EXPECT_EQ(actual_vector, fc::common::unhex(hash).value());
}

INSTANTIATE_TEST_CASE_P(
    MurmurTest, MurmurTest,
    ::testing::Values(
        MurmurTest::pair("", "0000000000000000"),
        MurmurTest::pair("61", "85555565F6597889"),
        MurmurTest::pair("6162", "938B11EA16ED1B2E"),
        MurmurTest::pair("616263", "B4963F3F3FAD7867"),
        MurmurTest::pair("61626364", "B87BB7D64656CD4F"),
        MurmurTest::pair("6162636465", "2036D091F496BBB8"),
        MurmurTest::pair("616263646566", "E47D86BFACA3BF55"),
        MurmurTest::pair("61626364656667", "A6CD2F9FC09EE499"),
        MurmurTest::pair("6162636465666768", "CC8A0AB037EF8C02"),
        MurmurTest::pair("616263646566676869", "0547C0CFF13C7964"),
        MurmurTest::pair("6162636465666768696A", "B6C15B0D772F8C99"),
        MurmurTest::pair("6162636465666768696A6B", "A895D0B8DF789D02"),
        MurmurTest::pair("6162636465666768696A6B6C", "8EF39BB1E67AE194"),
        MurmurTest::pair("6162636465666768696A6B6C6D", "1648288DA7C0FA73"),
        MurmurTest::pair("6162636465666768696A6B6C6D6E", "91D094A7F5C375E0"),
        MurmurTest::pair("6162636465666768696A6B6C6D6E6F", "8ABE2451890C2FFB"),
        MurmurTest::pair("6162636465666768696A6B6C6D6E6F70", "C4CA3CA3224CB723"),
        MurmurTest::pair("6162636465666768696A6B6C6D6E6F7071", "7564747F88BDA657"),
        MurmurTest::pair("6162636465666768696A6B6C6D6E6F707172737475767778797A414243444546", "914A06A1FE095F15"),
        MurmurTest::pair("6162636465666768696A6B6C6D6E6F707172737475767778797A41424344454647", "34B6C57AB3B22CB1")));
