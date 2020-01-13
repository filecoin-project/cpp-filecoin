/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/invoker.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "vm/actor/cron_actor.hpp"
#include "vm_context_mock.hpp"

using fc::vm::VMExitCode;

TEST(InvokerTest, InvokeCron) {
  using namespace fc::vm::actor;

  Invoker invoker;
  fc::vm::MockVMContext vmc;

  EXPECT_OUTCOME_ERROR(VMExitCode(254), invoker.invoke({kAccountCodeCid}, vmc, 0, {}));
  EXPECT_OUTCOME_ERROR(VMExitCode(255), invoker.invoke({kEmptyObjectCid}, vmc, 0, {}));
  EXPECT_OUTCOME_ERROR(VMExitCode(255), invoker.invoke({kCronCodeCid}, vmc, 1000, {}));

  EXPECT_CALL(vmc, message())
      .WillRepeatedly(testing::Return(fc::vm::Message{kInitAddress}));
  EXPECT_OUTCOME_ERROR(CronActor::WRONG_CALL, invoker.invoke({kCronCodeCid}, vmc, 2, {}));
}
