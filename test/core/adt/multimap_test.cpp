/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "adt/multimap.hpp"
#include "common/outcome.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "testutil/literals.hpp"

struct Fixture : public ::testing::Test {
  std::shared_ptr<fc::storage::ipfs::IpfsDatastore> store_{
      std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  fc::adt::Multimap mmap_{store_};
};

/**
 * @given an empty Array container
 * @when it is saved to IPLD storage
 * @then its root CID equals to expected
 */
TEST_F(Fixture, BasicEmpty) {
  auto res = mmap_.flush();
  ASSERT_TRUE(res);
  // empty CID is generated on golang side
  auto cidEmpty =
      "0171a0e4022018fe6acc61a3a36b0c373c4a3a8ea64b812bf2ca9b528050909c78d408558a0c"_cid;
  ASSERT_EQ(res.value(), cidEmpty);
}

TEST_F(Fixture, ValuesLoading) {
  const auto kKey = "my key";
  std::vector<gsl::span<const uint8_t>> values{
      "facade"_unhex, "cafe"_unhex, "babe"_unhex};
  for (const auto &v : values) {
    EXPECT_OUTCOME_TRUE_1(mmap_.add(kKey, v));
  }
  EXPECT_OUTCOME_TRUE(root, mmap_.flush());

  //  fc::adt::Multimap map{store_, root};
  auto index = 0;
  EXPECT_OUTCOME_TRUE_1(
      mmap_.visit(kKey, [&](const fc::adt::Multimap::Value &value) {
        std::cout << value << std::endl;
        EXPECT_EQ(values[index], gsl::make_span(value));
        ++index;
        return fc::outcome::success();
      }))
  EXPECT_EQ(index, values.size());
  EXPECT_OUTCOME_TRUE_1(mmap_.flush());
}

//
///**
// * @given an Array container sequentially filled with three values
// * @when "foreach" over the values is requested
// * @then the elements get accessed in the same order as they were appended
// */
// TEST_F(Fixture, OrderIsPreserved) {
//  std::vector<gsl::span<const uint8_t>> values{
//      "facade"_unhex, "cafe"_unhex, "babe"_unhex};
//  for (const auto &v : values) {
//    auto res = array_.append(v);
//    ASSERT_TRUE(res) << res.error().message();
//  }
//  auto index = 0;
//  EXPECT_OUTCOME_TRUE_1(array_.visit([&](const fc::adt::Array::Value &value) {
//    EXPECT_EQ(values[index], gsl::make_span(value));
//    ++index;
//    return fc::outcome::success();
//  }))
//  EXPECT_EQ(index, values.size());
//  EXPECT_OUTCOME_TRUE_1(array_.flush());
//}
