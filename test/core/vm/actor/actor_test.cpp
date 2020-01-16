/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/actor.hpp"

#include <gtest/gtest.h>
#include "testutil/cbor.hpp"

/**
 * @given Go constants
 * @when Compare to C++ constants
 * @then Equal
 */
TEST(ActorTest, CidGoCompatibility) {
  EXPECT_EQ(fc::vm::actor::kAccountCodeCid,
            "0155000d66696c2f312f6163636f756e74"_cid);
  EXPECT_EQ(fc::vm::actor::kCronCodeCid, "0155000a66696c2f312f63726f6e"_cid);
  EXPECT_EQ(fc::vm::actor::kStoragePowerCodeCid,
            "0155000b66696c2f312f706f776572"_cid);
  EXPECT_EQ(fc::vm::actor::kStorageMarketCodeCid,
            "0155000c66696c2f312f6d61726b6574"_cid);
  EXPECT_EQ(fc::vm::actor::kStorageMinerCodeCid,
            "0155000b66696c2f312f6d696e6572"_cid);
  EXPECT_EQ(fc::vm::actor::kMultisigCodeCid,
            "0155000e66696c2f312f6d756c7469736967"_cid);
  EXPECT_EQ(fc::vm::actor::kInitCodeCid, "0155000a66696c2f312f696e6974"_cid);
  EXPECT_EQ(fc::vm::actor::kPaymentChannelCodeCid,
            "0155000b66696c2f312f7061796368"_cid);

  EXPECT_EQ(
      fc::vm::actor::kEmptyObjectCid,
      "01711220c19a797fa1fd590cd2e5b42d1cf5f246e29b91684e2f87404b81dc345c7a56a0"_cid);
}

/** Actor CBOR encoding and decoding */
TEST(ActorTest, ActorCbor) {
  fc::vm::actor::Actor actor{
      fc::vm::actor::kAccountCodeCid,
      fc::vm::actor::ActorSubstateCID{fc::vm::actor::kCronCodeCid},
      3,
      5};
  expectEncodeAndReencode(
      actor,
      "84d82a52000155000d66696c2f312f6163636f756e74d82a4f000155000a66696c2f312f63726f6e03420005"_unhex);
}
