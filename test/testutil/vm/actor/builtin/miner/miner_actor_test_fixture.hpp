/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"

#include "vm/actor/builtin/states/miner_actor_state.hpp"

namespace fc::testutil::vm::actor::builtin::miner {
  using BaseMinerActorState = fc::vm::actor::builtin::states::MinerActorState;

  template <class State>
  class MinerActorTestFixture : public ActorTestFixture<State> {
   public:
    using ActorTestFixture<State>::ipld;
    using ActorTestFixture<State>::state_manager;
    using ActorTestFixture<State>::state;

    void SetUp() override {
      ActorTestFixture<State>::SetUp();

      EXPECT_CALL(*state_manager, createMinerActorState(testing::_))
          .WillRepeatedly(testing::Invoke([&](auto) {
            auto s = std::make_shared<State>();
            ipld->load(*s);
            return std::static_pointer_cast<BaseMinerActorState>(s);
          }));

      EXPECT_CALL(*state_manager, getMinerActorState())
          .WillRepeatedly(testing::Invoke([&]() {
            EXPECT_OUTCOME_TRUE(cid, ipld->setCbor(state));
            EXPECT_OUTCOME_TRUE(current_state,
                                ipld->template getCbor<State>(cid));
            auto s = std::make_shared<State>(current_state);
            return std::static_pointer_cast<BaseMinerActorState>(s);
          }));
    }
  };
}  // namespace fc::testutil::vm::actor::builtin::miner
