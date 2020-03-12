/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset_key.hpp"

#include <gtest/gtest.h>
#include "common/hexutil.hpp"
#include "testutil/cbor.hpp"

struct TipsetKeyTest : public ::testing::Test {
  using CID = fc::CID;
  using TipsetKey = fc::primitives::tipset::TipsetKey;

  void SetUp() override {
    std::vector<uint8_t> v1 = {'a'};
    std::vector<uint8_t> v2 = {'b'};
    std::vector<uint8_t> v3 = {'c'};
    EXPECT_OUTCOME_TRUE(c1, fc::common::getCidOf(v1));
    EXPECT_OUTCOME_TRUE(c2, fc::common::getCidOf(v2));
    EXPECT_OUTCOME_TRUE(c3, fc::common::getCidOf(v3));

    key2 = TipsetKey::create({c1}).value();
    key3 = TipsetKey::create({c1, c2}).value();
    key4 = TipsetKey::create({c1, c2, c3}).value();
  }

  boost::optional<TipsetKey> key1;
  boost::optional<TipsetKey> key2;
  boost::optional<TipsetKey> key3;
  boost::optional<TipsetKey> key4;
};

/**
 * This test is a usage example,
 * Anyway the implementation doesn't meet lotus version,
 * since we don't have cid.ToString() for CID V1 yet
 */
TEST_F(TipsetKeyTest, DISABLED_ToPrettyStringSuccess) {
  std::cout << key1->toPrettyString() << std::endl;
  std::cout << key2->toPrettyString() << std::endl;
  std::cout << key3->toPrettyString() << std::endl;
  std::cout << key4->toPrettyString() << std::endl;
}

/**
 * @given we have different tipset keys
 * @when call toBytes() method and calculate hex representation
 * @then the representation meets lotus-calculated value
 */
TEST_F(TipsetKeyTest, ToBytesSuccess) {
  EXPECT_OUTCOME_TRUE(vec2, key2->toBytes());
  EXPECT_OUTCOME_TRUE(vec3, key3->toBytes());
  EXPECT_OUTCOME_TRUE(vec4, key4->toBytes());
  ASSERT_EQ(fc::common::hex_lower(vec2),
            "0171a0e402208928aae63c84d87ea098564d1e03ad813f107add474e56aedd2863"
            "49c0c03ea4");
  ASSERT_EQ(fc::common::hex_lower(vec3),
            "0171a0e402208928aae63c84d87ea098564d1e03ad813f107add474e56aedd2863"
            "49c0c03ea40171a0e402206e5c1f45cbaf19f94230ba3501c378a5335af71a331b"
            "5b5aed62792332288dc3");
  ASSERT_EQ(fc::common::hex_lower(vec4),
            "0171a0e402208928aae63c84d87ea098564d1e03ad813f107add474e56aedd2863"
            "49c0c03ea40171a0e402206e5c1f45cbaf19f94230ba3501c378a5335af71a331b"
            "5b5aed62792332288dc30171a0e40220ed5402299a6208014e0f5f25ae6ca3badd"
            "dc95db67dce164cb8aa086bd48978a");
}

/**
 * @given set of non-equivalent leys key1, key2, key3, key4
 * @when get their bytes representation @and compare representations
 * @then representation don't match
 */
TEST_F(TipsetKeyTest, BytesRepresentationsDontMatch) {
  auto &&repr1 = key1->toBytes();
  auto &&repr2 = key2->toBytes();
  auto &&repr3 = key3->toBytes();
  auto &&repr4 = key4->toBytes();

  ASSERT_NE(repr1, repr2);
  ASSERT_NE(repr2, repr3);
  ASSERT_NE(repr3, repr4);
  ASSERT_NE(repr4, repr1);
  ASSERT_NE(repr1, repr3);
  ASSERT_NE(repr2, repr4);
}

/**
 * @given set of non-equivalent leys key1, key2, key3, key4
 * @when get their string representation @and compare representations
 * @then representation don't match
 */
TEST_F(TipsetKeyTest, StringRepresentationsDontMatch) {
  auto &&repr1 = key1->toPrettyString();
  auto &&repr2 = key2->toPrettyString();
  auto &&repr3 = key3->toPrettyString();
  auto &&repr4 = key4->toPrettyString();

  ASSERT_NE(repr1, repr2);
  ASSERT_NE(repr2, repr3);
  ASSERT_NE(repr3, repr4);
  ASSERT_NE(repr4, repr1);
  ASSERT_NE(repr1, repr3);
  ASSERT_NE(repr2, repr4);
}

/**
 * @given set of different keys
 * @when compare keys to themselves
 * @then result is success
 */
TEST_F(TipsetKeyTest, EqualSuccess) {
  ASSERT_EQ(key1, key1);
  ASSERT_EQ(key2, key2);
  ASSERT_EQ(key3, key3);
  ASSERT_EQ(key4, key4);
}

/**
 * @given set of different keys
 * @when check that different keys are not equal
 * @then result is success
 */
TEST_F(TipsetKeyTest, NotEqualSuccess) {
  ASSERT_NE(key1, key2);
  ASSERT_NE(key2, key3);
  ASSERT_NE(key3, key4);
  ASSERT_NE(key4, key1);
  ASSERT_NE(key1, key3);
  ASSERT_NE(key2, key4);
}
