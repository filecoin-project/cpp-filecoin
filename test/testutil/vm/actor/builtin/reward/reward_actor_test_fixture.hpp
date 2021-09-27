/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"

#include "vm/actor/builtin/states/reward/reward_actor_state.hpp"

namespace fc::testutil::vm::actor::builtin::reward {
  using fc::vm::actor::kSystemActorAddress;
  using fc::vm::actor::builtin::states::RewardActorState;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using testing::Eq;
  using testing::Return;

  static const TokenAmount kEpochZeroReward{"36266264293777134739"};

  class RewardActorTestFixture : public ActorTestFixture<RewardActorState> {
   public:
    using ActorTestFixture<RewardActorState>::runtime;
    using ActorTestFixture<RewardActorState>::callerIs;

    void setCurrentBalance(const TokenAmount &balance) {
      const Address receiver = Address::makeFromId(1001);
      EXPECT_CALL(runtime, getCurrentReceiver())
          .WillRepeatedly(Return(receiver));
      EXPECT_CALL(runtime, getBalance(Eq(receiver)))
          .WillRepeatedly(Return(outcome::success(balance)));
    }

    template <typename Constructor>
    void constructRewardActor(const StoragePower &start_realized_power = 0) {
      callerIs(kSystemActorAddress);
      EXPECT_OUTCOME_TRUE_1(Constructor::call(runtime, start_realized_power));
    }
  };
}  // namespace fc::testutil::vm::actor::builtin::reward
