/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "codec/cbor/light_reader/miner_actor_reader.hpp"
#include "codec/cbor/light_reader/storage_power_actor_reader.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/ipld2.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_state.hpp"

namespace fc::codec::cbor::light_reader {
  using primitives::RleBitset;
  using primitives::sector::RegisteredSealProof;
  using storage::ipfs::InMemoryDatastore;
  using storage::ipfs::IpfsDatastore;
  using storage::ipld::IpldIpld2;
  using vm::actor::ActorVersion;
  using vm::actor::builtin::states::ChainEpochKeyer;
  using vm::actor::builtin::states::State;
  using vm::actor::builtin::states::storage_power::ClaimV0;
  using vm::actor::builtin::states::storage_power::ClaimV2;
  using vm::actor::builtin::types::miner::Deadlines;
  using vm::actor::builtin::types::miner::MinerInfo;
  using vm::actor::builtin::types::miner::VestingFunds;
  using vm::actor::builtin::types::storage_power::CronEvent;
  using MinerActorStateV0 = vm::actor::builtin::v0::miner::MinerActorState;
  using MinerActorStateV2 = vm::actor::builtin::v2::miner::MinerActorState;
  using PowerActorStateV0 =
      vm::actor::builtin::v0::storage_power::PowerActorState;
  using PowerActorStateV2 =
      vm::actor::builtin::v2::storage_power::PowerActorState;

  class LightActorReader : public ::testing::Test {
   public:
    template <typename T>
    T makeSomeMinerActorState() {
      T state;
      ipld->load(state);
      MinerInfo info;
      info.window_post_partition_sectors = 100;
      info.sector_size = 101;
      EXPECT_OUTCOME_TRUE_1(state.setInfo(ipld, info));
      VestingFunds vesting_funds;
      EXPECT_OUTCOME_TRUE_1(state.vesting_funds.set(vesting_funds));
      RleBitset allocated_sectors;
      EXPECT_OUTCOME_TRUE_1(state.allocated_sectors.set(allocated_sectors));
      Deadlines deadlines;
      EXPECT_OUTCOME_TRUE_1(state.deadlines.set(deadlines));
      return state;
    }

   protected:
    IpldPtr ipld = std::make_shared<InMemoryDatastore>();
    LightIpldPtr light_ipld = std::make_shared<IpldIpld2>(ipld);
  };

  /**
   * @given Miner Actor V0 State with fields set (miner_info, sectors,
   * deadlines)
   * @when parse and extract fields
   * @then correct CID returned
   */
  TEST_F(LightActorReader, MinerActorV0) {
    auto state = makeSomeMinerActorState<MinerActorStateV0>();
    EXPECT_OUTCOME_TRUE(state_root, ipld->setCbor(state));

    CID expected_miner_info = state.miner_info;
    CID expected_sectors = state.sectors.amt.cid();
    CID expected_deadlines = state.deadlines;
    Hash256 actual_miner_info;
    Hash256 actual_sectors;
    Hash256 actual_deadlines;
    EXPECT_TRUE(readMinerActorInfo(light_ipld,
                                   *asBlake(state_root),
                                   ActorVersion::kVersion0,
                                   actual_miner_info,
                                   actual_sectors,
                                   actual_deadlines));
    EXPECT_EQ(*asBlake(expected_miner_info), actual_miner_info);
    EXPECT_EQ(*asBlake(expected_sectors), actual_sectors);
    EXPECT_EQ(*asBlake(expected_deadlines), actual_deadlines);
  }

  /**
   * @given Miner Actor V2 State with fields set (miner_info, sectors,
   * deadlines)
   * @when parse and extract fields
   * @then correct CID returned
   */
  TEST_F(LightActorReader, MinerActorV2) {
    auto state = makeSomeMinerActorState<MinerActorStateV2>();
    EXPECT_OUTCOME_TRUE(state_root, ipld->setCbor(state));

    CID expected_miner_info = state.miner_info;
    CID expected_sectors = state.sectors.amt.cid();
    CID expected_deadlines = state.deadlines;
    Hash256 actual_miner_info;
    Hash256 actual_sectors;
    Hash256 actual_deadlines;
    EXPECT_TRUE(readMinerActorInfo(light_ipld,
                                   *asBlake(state_root),
                                   ActorVersion::kVersion2,
                                   actual_miner_info,
                                   actual_sectors,
                                   actual_deadlines));
    EXPECT_EQ(*asBlake(expected_miner_info), actual_miner_info);
    EXPECT_EQ(*asBlake(expected_sectors), actual_sectors);
    EXPECT_EQ(*asBlake(expected_deadlines), actual_deadlines);
  }

  /**
   * @given Storage Power Actor V0 State with claims set
   * @when parse and extract claims
   * @then correct claims CID returned
   */
  TEST_F(LightActorReader, PowerActorV0) {
    PowerActorStateV0 state;
    ipld->load(state);
    primitives::address::Address address =
        primitives::address::Address::makeFromId(100);
    ClaimV0 claim;
    claim.raw_power = 101;
    claim.qa_power = 102;
    EXPECT_OUTCOME_TRUE_1(state.claims0.set(address, claim));
    EXPECT_OUTCOME_TRUE(state_root, ipld->setCbor(state));

    auto expected = state.claims0.hamt.cid();
    Hash256 actual;
    EXPECT_TRUE(readStoragePowerActorClaims(
        light_ipld, *asBlake(state_root), ActorVersion::kVersion0, actual));
    EXPECT_EQ(*asBlake(expected), actual);
  }

  /**
   * @given Storage Power Actor V2 State with claims set
   * @when parse and extract claims
   * @then correct claims CID returned
   */
  TEST_F(LightActorReader, PowerActorV2) {
    PowerActorStateV2 state;
    ipld->load(state);
    primitives::address::Address address =
        primitives::address::Address::makeFromId(100);
    ClaimV2 claim;
    claim.seal_proof_type = RegisteredSealProof::kStackedDrg2KiBV1;
    claim.raw_power = 101;
    claim.qa_power = 102;
    EXPECT_OUTCOME_TRUE_1(state.claims2.set(address, claim));
    EXPECT_OUTCOME_TRUE(state_root, ipld->setCbor(state));

    auto expected = state.claims2.hamt.cid();
    Hash256 actual;
    EXPECT_TRUE(readStoragePowerActorClaims(
        light_ipld, *asBlake(state_root), ActorVersion::kVersion2, actual));
    EXPECT_EQ(*asBlake(expected), actual);
  }

}  // namespace fc::codec::cbor::light_reader
