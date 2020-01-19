/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/cron_actor.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "vm/actor/actor.hpp"
#include "testutil/mocks/vm/vm_context_mock.hpp"

using namespace fc::vm;

/**
 * @given Virtual Machine context
 * @when get message not from CronActor
 * @then error WRONG_CALL
 */
TEST(CronActorTest, WrongSender) {
  Message message_wrong_sender{actor::kInitAddress};
  MockVMContext vmctx;
  actor::Actor actor;
  EXPECT_CALL(vmctx, message())
      .WillRepeatedly(testing::Return(message_wrong_sender));
  EXPECT_OUTCOME_FALSE(err, actor::CronActor::epochTick(actor, vmctx, {}));
  ASSERT_EQ(err, actor::CronActor::WRONG_CALL);
}

/**
 * @given Virtual Machine context
 * @when get message from CronActor
 * @then success
 */
TEST(CronActorTest, Correct) {
  Message message{actor::kCronAddress};
  MockVMContext vmctx;
  actor::Actor actor;
  EXPECT_CALL(vmctx, message()).WillRepeatedly(testing::Return(message));
  EXPECT_CALL(vmctx,
              send(actor::kStoragePowerAddress,
                   actor::SpaMethods::CHECK_PROOF_SUBMISSIONS,
                   actor::BigInt(0),
                   std::vector<uint8_t>()))
      .WillRepeatedly(testing::Return(fc::outcome::success()));
  EXPECT_OUTCOME_TRUE_1(actor::CronActor::epochTick(actor, vmctx, {}));
}
