/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/invoker.hpp"

#include <gtest/gtest.h>
#include "testutil/cbor.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/cron_actor.hpp"
#include "vm_context_mock.hpp"

using fc::vm::VMExitCode;

TEST(InvokerTest, InvokeCron) {
  using namespace fc::vm::actor;

  Invoker invoker;
  fc::vm::MockVMContext vmc;

  EXPECT_OUTCOME_ERROR(Invoker::CANT_INVOKE_ACCOUNT_ACTOR, invoker.invoke({kAccountCodeCid}, vmc, 0, {}));
  EXPECT_OUTCOME_ERROR(Invoker::NO_CODE_OR_METHOD, invoker.invoke({kEmptyObjectCid}, vmc, 0, {}));
  EXPECT_OUTCOME_ERROR(Invoker::NO_CODE_OR_METHOD, invoker.invoke({kCronCodeCid}, vmc, 1000, {}));

  EXPECT_CALL(vmc, message())
      .WillRepeatedly(testing::Return(fc::vm::Message{kInitAddress}));
  EXPECT_OUTCOME_ERROR(CronActor::WRONG_CALL, invoker.invoke({kCronCodeCid}, vmc, 2, {}));
}

TEST(InvokerTest, DecodeActorParams) {
  using fc::vm::actor::decodeActorParams;

  EXPECT_OUTCOME_ERROR(fc::vm::actor::DECODE_ACTOR_PARAMS_ERROR, decodeActorParams<int>("80"_unhex));
  EXPECT_OUTCOME_EQ(decodeActorParams<int>("03"_unhex), 3);
}
