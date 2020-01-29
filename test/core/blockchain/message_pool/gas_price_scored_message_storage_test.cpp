/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "blockchain/message_pool/impl/gas_price_scored_message_storage.hpp"
#include "blockchain/message_pool/message_pool_error.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/vm/message/message_test_util.hpp"

using fc::blockchain::message_pool::GasPriceScoredMessageStorage;
using fc::blockchain::message_pool::MessagePoolError;
using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::primitives::address::Network;
using fc::vm::message::MethodNumber;
using fc::vm::message::MethodParams;
using fc::vm::message::SignedMessage;
using fc::vm::message::UnsignedMessage;
using PrivateKey = std::array<uint8_t, 32>;

class GasPricedScoredMessageStorageTest : public testing::Test {
 public:
  GasPriceScoredMessageStorage message_storage;

  Address to{Network::TESTNET, 1001};
  Address from{Network::TESTNET, 1002};
  UnsignedMessage unsigned_message{
      to,                     // to Address
      from,                   // from Address
      0,                      // nonce
      BigInt(1),              // transfer value
      BigInt(0),              // gasPrice
      BigInt(1),              // gasLimit
      MethodNumber{0},        // method num
      MethodParams{""_unhex}  // method params
  };
  PrivateKey bls_private_key =
      "8e8c5263df0022d8e29cab943d57d851722c38ee1dbe7f8c29c0498156496f29"_blob32;
  SignedMessage message =
      signMessageBls(unsigned_message, bls_private_key).value();
};

/**
 * @given MessageStorage is empty and message
 * @when remove message is called
 * @then nothing happens
 */
TEST_F(GasPricedScoredMessageStorageTest, RemoveNotExists) {
  message_storage.remove(message);
}

/**
 * @given MessageStorage with message stored
 * @when add again is called
 * @then Error "already in pool" is returned
 */
TEST_F(GasPricedScoredMessageStorageTest, AddTwice) {
  EXPECT_OUTCOME_TRUE_1(message_storage.put(message));
  EXPECT_OUTCOME_ERROR(MessagePoolError::MESSAGE_ALREADY_IN_POOL,
                       message_storage.put(message));
}

/**
 * @given MessageStorage is empty
 * @when get top scored is called
 * @then Empty list returned
 */
TEST_F(GasPricedScoredMessageStorageTest, GetEmty) {
  auto empty = message_storage.getTopScored(1);
  ASSERT_TRUE(empty.empty());
}

/**
 * @given MessageStorage contains 1 message
 * @when get 5 messages is called
 * @then Only 1 message returned
 */
TEST_F(GasPricedScoredMessageStorageTest, GetMoreThanExists) {
  EXPECT_OUTCOME_TRUE_1(message_storage.put(message));
  auto messages = message_storage.getTopScored(5);
  ASSERT_EQ(messages.size(), 1);
}

/**
 * @given MessageStorage populated with messages with different gas price
 * @when get scored is called
 * @then returned messages scored on gas price
 */
TEST_F(GasPricedScoredMessageStorageTest, SunnyDay) {
  // populate
  BigInt gas_price1{1};
  BigInt gas_price2{2};
  BigInt gas_price3{3};
  UnsignedMessage unsigned_message1{
      to,                     // to Address
      from,                   // from Address
      0,                      // nonce
      BigInt(1),              // transfer value
      gas_price1,             // gasPrice
      BigInt(1),              // gasLimit
      MethodNumber{0},        // method num
      MethodParams{""_unhex}  // method params
  };
  SignedMessage message1 =
      signMessageBls(unsigned_message1, bls_private_key).value();
  EXPECT_OUTCOME_TRUE_1(message_storage.put(message1));

  UnsignedMessage unsigned_message2{
      to,                     // to Address
      from,                   // from Address
      1,                      // nonce
      BigInt(1),              // transfer value
      gas_price1,             // gasPrice
      BigInt(1),              // gasLimit
      MethodNumber{0},        // method num
      MethodParams{""_unhex}  // method params
  };
  SignedMessage message2 =
      signMessageBls(unsigned_message2, bls_private_key).value();
  EXPECT_OUTCOME_TRUE_1(message_storage.put(message2));

  UnsignedMessage unsigned_message3{
      to,                     // to Address
      from,                   // from Address
      2,                      // nonce
      BigInt(1),              // transfer value
      gas_price3,             // gasPrice
      BigInt(1),              // gasLimit
      MethodNumber{0},        // method num
      MethodParams{""_unhex}  // method params
  };
  SignedMessage message3 =
      signMessageBls(unsigned_message3, bls_private_key).value();
  EXPECT_OUTCOME_TRUE_1(message_storage.put(message3));

  UnsignedMessage unsigned_message4{
      to,                     // to Address
      from,                   // from Address
      3,                      // nonce
      BigInt(1),              // transfer value
      gas_price2,             // gasPrice
      BigInt(1),              // gasLimit
      MethodNumber{0},        // method num
      MethodParams{""_unhex}  // method params
  };
  SignedMessage message4 =
      signMessageBls(unsigned_message4, bls_private_key).value();
  EXPECT_OUTCOME_TRUE_1(message_storage.put(message4));

  auto top = message_storage.getTopScored(3);
  ASSERT_EQ(top.size(), 3);
  ASSERT_EQ(top[0].message.gasPrice, gas_price3);
  ASSERT_EQ(top[1].message.gasPrice, gas_price2);
  ASSERT_EQ(top[2].message.gasPrice, gas_price1);
}
