/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdexcept>

#include <gtest/gtest.h>

#include "filecoin/adt/array.hpp"
#include "filecoin/common/outcome.hpp"
#include "filecoin/storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "testutil/literals.hpp"

struct Fixture : public ::testing::Test {
  using Value = fc::adt::Array::Value;
  std::shared_ptr<fc::storage::ipfs::IpfsDatastore> store_{
      std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  fc::adt::Array array_{store_};
  std::vector<Value> values_{
      Value{"06"_unhex}, Value{"07"_unhex}, Value{"08"_unhex}};

  fc::outcome::result<void> appendValues(fc::adt::Array &array) {
    for (auto &v : values_) {
      auto res = array.append(v);
      if (not res) {
        return res.error();
      }
    }
    return fc::outcome::success();
  }

  void checkValues(fc::adt::Array &array) {
    auto index = 0u;
    EXPECT_OUTCOME_TRUE_1(array.visit([&](const fc::adt::Array::Value &value) {
      EXPECT_EQ(values_[index], gsl::make_span(value));
      ++index;
      return fc::outcome::success();
    }))
    EXPECT_EQ(index, values_.size());
  }
};

/**
 * @given an empty Array container
 * @when it is saved to IPLD storage
 * @then its root CID equals to expected
 */
TEST_F(Fixture, BasicEmpty) {
  auto res = array_.flush();
  ASSERT_TRUE(res);
  // empty CID is generated on golang side
  auto cidEmpty =
      "0171a0e4022001cd927fdccd7938faba323e32e70c44541b8a83f5dc941d90866565ef5af14a"_cid;
  ASSERT_EQ(res.value(), cidEmpty);
}

/**
 * @given an Array container sequentially filled with three values
 * @when "foreach" over the values is requested
 * @then the elements get accessed in the same order as they were appended
 */
TEST_F(Fixture, OrderIsPreserved) {
  EXPECT_OUTCOME_TRUE_1(appendValues(array_));
  checkValues(array_);
  EXPECT_OUTCOME_TRUE_1(array_.flush());
}

/**
 * @given an Array initialized with three values and saved
 * @when another Array is initialized with the same root CID
 * @then all the expected values can be accessed in expected order
 */
TEST_F(Fixture, AccessByCid) {
  EXPECT_OUTCOME_TRUE_1(appendValues(array_));
  EXPECT_OUTCOME_TRUE(array_root, array_.flush());
  fc::adt::Array new_array{store_, array_root};
  checkValues(new_array);
}

/**
 * @given An initialized Array
 * @when it is saved and elements are accessed via AMT abstraction
 * @then elements order and indices are as expected
 */
TEST_F(Fixture, UnderlyingAmt) {
  EXPECT_OUTCOME_TRUE_1(appendValues(array_));
  EXPECT_OUTCOME_TRUE(array_root, array_.flush());

  auto index = 0u;
  fc::storage::amt::Amt amt{store_, array_root};
  EXPECT_OUTCOME_TRUE_1(amt.visit(
      [&index, this](auto &&k, auto &&v) -> fc::outcome::result<void> {
        if (k != index) {
          return std::errc::bad_address;
        }
        if (values_[index] != v) {
          return std::errc::bad_message;
        }
        ++index;
        return fc::outcome::success();
      }));
}
