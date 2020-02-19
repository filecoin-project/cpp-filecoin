/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor_method.hpp"

using fc::CID;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsDatastore;
using fc::vm::VMExitCode;
using fc::vm::actor::Actor;
using fc::vm::actor::ActorSubstateCID;
using fc::vm::actor::encodeActorParams;
using fc::vm::actor::kCronAddress;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::builtin::storage_power::ConstructParameters;
using fc::vm::actor::builtin::storage_power::StoragePowerActor;
using fc::vm::actor::builtin::storage_power::StoragePowerActorMethods;
using fc::vm::actor::builtin::storage_power::StoragePowerActorState;
using fc::vm::runtime::MockRuntime;
using ::testing::_;

class StoragePowerActorTest : public ::testing::Test {
 public:
  // Captures CID in mock call
  fc::outcome::result<void> captureCid(const ActorSubstateCID &cid) {
    captured_cid = cid;
    return fc::outcome::success();
  }

  // Returnes captured CID value
  ActorSubstateCID getCapturedCid() {
    return captured_cid;
  }

  Actor caller;
  std::shared_ptr<IpfsDatastore> datastore =
      std::make_shared<InMemoryDatastore>();
  MockRuntime runtime;

 private:
  ActorSubstateCID captured_cid;
};

/**
 * @given runtime and StoragePowerActor
 * @when constructor is called with caller actor different from SystemActor
 * @then Error returned
 */
TEST_F(StoragePowerActorTest, ConstructorWrongCaller) {
  ConstructParameters params;
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  // return != kInitAddress
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(kCronAddress));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER,
      StoragePowerActorMethods::construct(caller, runtime, encoded_params));
}

/**
 * @given runtime and StoragePowerActor
 * @when constructor is called
 * @then empty state is created
 */
TEST_F(StoragePowerActorTest, Constructor) {
  ConstructParameters params;
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(kInitAddress));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  // commit and capture state CID
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(::testing::Invoke(this, &StoragePowerActorTest::captureCid));

  EXPECT_OUTCOME_TRUE_1(
      StoragePowerActorMethods::construct(caller, runtime, encoded_params));

  // check state
  ActorSubstateCID state_cid = getCapturedCid();
  EXPECT_OUTCOME_TRUE(state,
                      datastore->getCbor<StoragePowerActorState>(state_cid));
  StoragePowerActor actor(datastore, state);
  EXPECT_OUTCOME_TRUE(cron_events, actor.getCronEvents());
  EXPECT_TRUE(cron_events.empty());
  EXPECT_OUTCOME_TRUE(fault_miners, actor.getFaultMiners());
  EXPECT_TRUE(fault_miners.empty());
  EXPECT_OUTCOME_TRUE(claims, actor.getClaims());
  EXPECT_TRUE(claims.empty());
  EXPECT_OUTCOME_TRUE(miners, actor.getMiners());
  EXPECT_TRUE(miners.empty());
}
