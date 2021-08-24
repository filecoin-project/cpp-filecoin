/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"

#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/types/type_manager/type_manager.hpp"

namespace fc::testutil::vm::actor::builtin::miner {
  using ::fc::vm::actor::builtin::types::TypeManager;
  using ::fc::vm::actor::builtin::types::miner::MinerInfo;
  using ::fc::vm::actor::builtin::types::miner::SectorOnChainInfo;
  using ::fc::vm::actor::builtin::types::miner::VestingFunds;
  using primitives::RleBitset;
  using primitives::address::Address;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using BaseMinerActorState = fc::vm::actor::builtin::states::MinerActorState;

  template <class State>
  class MinerActorTestFixture : public ActorTestFixture<State> {
   public:
    using ActorTestFixture<State>::ipld;
    using ActorTestFixture<State>::runtime;
    using ActorTestFixture<State>::state;

    void loadState(State &s) {
      cbor_blake::cbLoadT(ipld, s);
    }

    void initEmptyState() {
      EXPECT_OUTCOME_TRUE(empty_amt_cid,
                          state.precommitted_setctors_expiry.amt.flush());

      state.miner_info.cid = empty_amt_cid;

      RleBitset allocated_sectors;
      EXPECT_OUTCOME_TRUE_1(state.allocated_sectors.set(allocated_sectors));
      EXPECT_OUTCOME_TRUE(
          deadlines, TypeManager::makeEmptyDeadlines(runtime, empty_amt_cid));
      EXPECT_OUTCOME_TRUE_1(state.deadlines.set(deadlines));

      VestingFunds vesting_funds;
      EXPECT_OUTCOME_TRUE_1(state.vesting_funds.set(vesting_funds));

      state.sectors = {empty_amt_cid, ipld};
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
