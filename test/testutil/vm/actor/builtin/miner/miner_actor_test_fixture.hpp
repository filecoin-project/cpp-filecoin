/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"

#include "vm/actor/builtin/states/miner_actor_state.hpp"
#include "vm/actor/builtin/types/type_manager/type_manager.hpp"

namespace fc::testutil::vm::actor::builtin::miner {
  using ::fc::vm::actor::builtin::types::miner::MinerInfo;
  using ::fc::vm::actor::builtin::types::miner::SectorOnChainInfo;
  using ::fc::vm::actor::builtin::types::miner::VestingFunds;
  using primitives::RleBitset;
  using primitives::address::Address;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using types::TypeManager;
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
            loadState(*s);
            return std::static_pointer_cast<BaseMinerActorState>(s);
          }));

      EXPECT_CALL(*state_manager, getMinerActorState())
          .WillRepeatedly(testing::Invoke([&]() {
            EXPECT_OUTCOME_TRUE(cid, setCbor(ipld, state));
            EXPECT_OUTCOME_TRUE(current_state, getCbor<State>(ipld, cid));
            auto s = std::make_shared<State>(current_state);
            return std::static_pointer_cast<BaseMinerActorState>(s);
          }));
    }

    void loadState(State &s) {
      cbor_blake::cbLoadT(ipld, s);
    }

    void initEmptyState() {
      EXPECT_OUTCOME_TRUE(empty_amt_cid,
                          state.precommitted_setctors_expiry.amt.flush());

      state.miner_info = empty_amt_cid;

      RleBitset allocated_sectors;
      EXPECT_OUTCOME_TRUE_1(state.allocated_sectors.set(allocated_sectors));
      EXPECT_OUTCOME_TRUE(deadlines,
                          state.makeEmptyDeadlines(ipld, empty_amt_cid));
      EXPECT_OUTCOME_TRUE_1(state.deadlines.set(deadlines));

      VestingFunds vesting_funds;
      EXPECT_OUTCOME_TRUE_1(state.vesting_funds.set(vesting_funds));

      state.sectors = adt::Array<SectorOnChainInfo>(empty_amt_cid, ipld);
    }

    void initDefaultMinerInfo() {
      std::vector<Address> control_addresses;
      control_addresses.emplace_back(control);

      EXPECT_OUTCOME_TRUE(
          miner_info,
          TypeManager::makeMinerInfo(runtime,
                                     owner,
                                     worker,
                                     control_addresses,
                                     {},
                                     {},
                                     RegisteredSealProof::kUndefined,
                                     RegisteredPoStProof::kUndefined));

      EXPECT_OUTCOME_TRUE_1(state.miner_info.set(miner_info));
    }

    Address owner{Address::makeFromId(100)};
    Address worker{Address::makeFromId(101)};
    Address control{Address::makeFromId(501)};
  };
}  // namespace fc::testutil::vm::actor::builtin::miner
