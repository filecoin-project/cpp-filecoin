/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/actor/builtin/states/miner/v3/miner_actor_state.hpp"

namespace fc::vm::actor::builtin {
  using runtime::MockRuntime;
  using states::makeEmptyMinerState;
  using states::MinerActorStatePtr;
  using storage::ipfs::IpfsDatastore;
  using types::miner::kPreCommitChallengeDelay;
  using types::miner::makeMinerInfo;

  inline MinerActorStatePtr makeActorState(
      const std::shared_ptr<IpfsDatastore> &ipld, ActorVersion actor_version) {
    MockRuntime runtime;
    EXPECT_CALL(runtime, getActorVersion())
        .WillRepeatedly(testing::Invoke([&]() { return actor_version; }));

    EXPECT_CALL(runtime, getIpfsDatastore())
        .WillRepeatedly(testing::Invoke([&]() { return ipld; }));

    EXPECT_OUTCOME_TRUE(actor_state, makeEmptyMinerState(runtime));
    std::vector<Address> control_addresses;
    control_addresses.emplace_back(Address::makeFromId(501));
    EXPECT_OUTCOME_TRUE(
        miner_info,
        makeMinerInfo(actor_version,
                      Address::makeFromId(100),
                      Address::makeFromId(101),
                      control_addresses,
                      {},
                      {},
                      api::RegisteredSealProof::kStackedDrg32GiBV1,
                      api::RegisteredPoStProof::kStackedDRG32GiBWindowPoSt));

    EXPECT_OUTCOME_TRUE_1(actor_state->miner_info.set(miner_info));
    return actor_state;
  }
}  // namespace fc::vm::actor::builtin
