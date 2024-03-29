/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"

#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"

namespace fc::testutil::vm::actor::builtin::miner {
  using fc::vm::actor::builtin::states::makeEmptyMinerState;
  using fc::vm::actor::builtin::states::MinerActorState;
  using fc::vm::actor::builtin::types::miner::makeMinerInfo;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;

  class MinerActorTestFixture : public ActorTestFixture<MinerActorState> {
   public:
    using ActorTestFixture<MinerActorState>::actor_version;
    using ActorTestFixture<MinerActorState>::ipld;
    using ActorTestFixture<MinerActorState>::runtime;
    using ActorTestFixture<MinerActorState>::state;

    void SetUp() override {
      ActorTestFixture<MinerActorState>::SetUp();

      EXPECT_CALL(runtime, getBalance(actor_address))
          .WillRepeatedly(testing::Invoke(
              [&](auto &) { return fc::outcome::success(balance); }));

      EXPECT_CALL(runtime, getCurrentReceiver())
          .WillRepeatedly(testing::Invoke([&]() { return actor_address; }));
    }

    void initEmptyState() {
      EXPECT_OUTCOME_TRUE(empty_state, makeEmptyMinerState(runtime));
      state = empty_state;
    }

    void initDefaultMinerInfo() {
      std::vector<Address> control_addresses;
      control_addresses.emplace_back(control);

      EXPECT_OUTCOME_TRUE(
          miner_info,
          makeMinerInfo(actor_version,
                        owner,
                        worker,
                        control_addresses,
                        {},
                        {},
                        RegisteredSealProof::kStackedDrg32GiBV1,
                        RegisteredPoStProof::kStackedDRG32GiBWindowPoSt));

      EXPECT_OUTCOME_TRUE_1(state->miner_info.set(miner_info));
    }

    Address owner{Address::makeFromId(100)};
    Address worker{Address::makeFromId(101)};
    Address actor_address{Address::makeFromId(102)};
    Address control{Address::makeFromId(501)};
    TokenAmount balance{0};
  };
}  // namespace fc::testutil::vm::actor::builtin::miner
