/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/cron_actor.hpp"

#include <gtest/gtest.h>
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor.hpp"

using namespace fc::vm;
using fc::vm::actor::MethodNumber;
using fc::vm::actor::MethodParams;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::MockRuntime;

/**
 * @given Virtual Machine context
 * @when get message not from CronActor
 * @then error WRONG_CALL
 */
TEST(CronActorTest, WrongSender) {
  auto message_wrong_sender = std::make_shared<UnsignedMessage>(
      UnsignedMessage{actor::kInitAddress, actor::kInitAddress});
  auto runtime = std::make_shared<fc::vm::runtime::MockRuntime>();
  actor::Actor actor;
  EXPECT_CALL(*runtime, getMessage())
      .WillOnce(testing::Return(message_wrong_sender));
  EXPECT_OUTCOME_FALSE(err, actor::CronActor::epochTick(actor, runtime, {}));
  ASSERT_EQ(err, actor::CronActor::WRONG_CALL);
}

/**
 * @given Virtual Machine context
 * @when get message from CronActor
 * @then success
 */
TEST(CronActorTest, Correct) {
  auto message = std::make_shared<UnsignedMessage>(
      UnsignedMessage{actor::kInitAddress, actor::kCronAddress});
  auto runtime = std::make_shared<fc::vm::runtime::MockRuntime>();
  actor::Actor actor;
  EXPECT_CALL(*runtime, getMessage()).WillOnce(testing::Return(message));
  EXPECT_CALL(*runtime,
              send(actor::kStoragePowerAddress,
                   MethodNumber{actor::SpaMethods::CHECK_PROOF_SUBMISSIONS},
                   MethodParams{},
                   actor::BigInt(0)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_OUTCOME_TRUE_1(actor::CronActor::epochTick(actor, runtime, {}));
}
