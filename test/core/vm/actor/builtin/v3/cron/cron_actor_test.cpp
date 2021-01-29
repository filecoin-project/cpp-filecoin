/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/cron/cron_actor.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/actor/builtin/v3/storage_power/storage_power_actor_export.hpp"

using namespace fc::vm;
using actor::MethodParams;
using actor::builtin::v3::cron::EpochTick;
using actor::builtin::v3::storage_power::OnEpochTickEnd;
using runtime::MockRuntime;

/**
 * @given Virtual Machine context
 * @when get message not from CronActor
 * @then error WRONG_CALL
 */
TEST(CronActorV3Test, WrongSender) {
  MockRuntime runtime;
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(actor::kInitAddress));
  EXPECT_OUTCOME_ERROR(ABORT_CAST(VMExitCode::kSysErrForbidden),
                       EpochTick::call(runtime, {}));
}

/**
 * @given Virtual Machine context
 * @when get message from CronActor
 * @then success
 */
TEST(CronActorV3Test, Correct) {
  MockRuntime runtime;
  auto ipld = std::make_shared<fc::storage::ipfs::InMemoryDatastore>();
  EXPECT_OUTCOME_TRUE(
      state,
      ipld->setCbor(actor::builtin::v3::cron::State{
          {{actor::kStoragePowerAddress, OnEpochTickEnd::Number}}}));
  EXPECT_CALL(runtime, getCurrentActorState()).WillOnce(testing::Return(state));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(ipld));
  EXPECT_CALL(runtime,
              send(actor::kStoragePowerAddress,
                   OnEpochTickEnd::Number,
                   MethodParams{},
                   fc::primitives::TokenAmount{0}))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(actor::kSystemActorAddress));
  EXPECT_OUTCOME_TRUE_1(EpochTick::call(runtime, {}));
}
