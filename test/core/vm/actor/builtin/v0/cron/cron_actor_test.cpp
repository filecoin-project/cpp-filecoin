/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/cron/cron_actor.hpp"
#include "vm/actor/builtin/states/cron/v0/cron_actor_state.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_export.hpp"

#include <gtest/gtest.h>

#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"

namespace fc::vm::actor::builtin::v0::cron {
  using actor::MethodParams;
  using actor::builtin::v0::cron::EpochTick;
  using actor::builtin::v0::storage_power::OnEpochTickEnd;
  using testutil::vm::actor::builtin::ActorTestFixture;

  struct CronActorTest : public ActorTestFixture<CronActorState> {
    void SetUp() override {
      ActorTestFixture<CronActorState>::SetUp();
      actor_version = ActorVersion::kVersion0;
      ipld->actor_version = actor_version;
    }
  };

  /**
   * @given Virtual Machine context
   * @when get message not from CronActor
   * @then error WRONG_CALL
   */
  TEST_F(CronActorTest, WrongSender) {
    callerIs(kInitAddress);
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         EpochTick::call(runtime, {}));
  }

  /**
   * @given Virtual Machine context
   * @when get message from CronActor
   * @then success
   */
  TEST_F(CronActorTest, Correct) {
    state.entries = {{kStoragePowerAddress, OnEpochTickEnd::Number}};
    callerIs(kSystemActorAddress);

    EXPECT_CALL(runtime,
                send(actor::kStoragePowerAddress,
                     OnEpochTickEnd::Number,
                     MethodParams{},
                     primitives::TokenAmount{0}))
        .WillOnce(testing::Return(fc::outcome::success()));

    EXPECT_OUTCOME_TRUE_1(EpochTick::call(runtime, {}));
  }
}  // namespace fc::vm::actor::builtin::v0::cron
