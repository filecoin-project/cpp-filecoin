/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "cbor_blake/ipld_any.hpp"
#include "codec/cbor/light_reader/miner_actor_reader.hpp"
#include "codec/cbor/light_reader/storage_power_actor_reader.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/storage_power/claim.hpp"
#include "vm/actor/builtin/types/type_manager/universal.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_state.hpp"

namespace fc::codec::cbor::light_reader {
  using primitives::RleBitset;
  using primitives::sector::RegisteredSealProof;
  using storage::ipfs::InMemoryDatastore;
  using vm::actor::ActorVersion;
  using vm::actor::builtin::states::ChainEpochKeyer;
  using vm::actor::builtin::types::Universal;
  using vm::actor::builtin::types::miner::Deadlines;
  using vm::actor::builtin::types::miner::MinerInfo;
  using vm::actor::builtin::types::miner::VestingFunds;
  using vm::actor::builtin::types::storage_power::Claim;
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
    T makeSomeMinerActorState(ActorVersion version) {
      T state;
      cbor_blake::cbLoadT(ipld, state);
      Universal<MinerInfo> miner_info{version};
      EXPECT_OUTCOME_TRUE_1(state.miner_info.set(miner_info));
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
    CbIpldPtr light_ipld = std::make_shared<AnyAsCbIpld>(ipld);
  };

  /**
   * @given Miner Actor V0 State with fields set (miner_info, sectors,
   * deadlines)
   * @when parse and extract fields
   * @then correct CID returned
   */
  TEST_F(LightActorReader, MinerActorV0) {
    const auto state =
        makeSomeMinerActorState<MinerActorStateV0>(ActorVersion::kVersion0);
    EXPECT_OUTCOME_TRUE(state_root, setCbor(ipld, state));

    const CID expected_miner_info = state.miner_info;
    const CID expected_sectors = state.sectors.amt.cid();
    const CID expected_deadlines = state.deadlines.cid;
    EXPECT_OUTCOME_TRUE(
        actual, readMinerActorInfo(light_ipld, *asBlake(state_root), true));
    const auto [actual_miner_info, actual_sectors, actual_deadlines] = actual;
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
    const auto state =
        makeSomeMinerActorState<MinerActorStateV2>(ActorVersion::kVersion2);
    EXPECT_OUTCOME_TRUE(state_root, setCbor(ipld, state));

    const CID expected_miner_info = state.miner_info;
    const CID expected_sectors = state.sectors.amt.cid();
    const CID expected_deadlines = state.deadlines.cid;
    EXPECT_OUTCOME_TRUE(
        actual, readMinerActorInfo(light_ipld, *asBlake(state_root), false));
    const auto [actual_miner_info, actual_sectors, actual_deadlines] = actual;
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
    cbor_blake::cbLoadT(ipld, state);
    primitives::address::Address address =
        primitives::address::Address::makeFromId(100);

    Universal<Claim> claim{ActorVersion::kVersion0};
    claim->raw_power = 101;
    claim->qa_power = 102;
    EXPECT_OUTCOME_TRUE_1(state.claims.set(address, claim));
    EXPECT_OUTCOME_TRUE(state_root, setCbor(ipld, state));

    auto expected = state.claims.hamt.cid();
    EXPECT_OUTCOME_TRUE(
        actual,
        readStoragePowerActorClaims(light_ipld, *asBlake(state_root), true));
    EXPECT_EQ(*asBlake(expected), actual);
  }

  /**
   * @given Storage Power Actor V2 State with claims set
   * @when parse and extract claims
   * @then correct claims CID returned
   */
  TEST_F(LightActorReader, PowerActorV2) {
    PowerActorStateV2 state;
    cbor_blake::cbLoadT(ipld, state);
    primitives::address::Address address =
        primitives::address::Address::makeFromId(100);

    Universal<Claim> claim{ActorVersion::kVersion2};
    claim->seal_proof_type = RegisteredSealProof::kStackedDrg2KiBV1;
    claim->raw_power = 101;
    claim->qa_power = 102;
    EXPECT_OUTCOME_TRUE_1(state.claims.set(address, claim));
    EXPECT_OUTCOME_TRUE(state_root, setCbor(ipld, state));

    auto expected = state.claims.hamt.cid();
    EXPECT_OUTCOME_TRUE(
        actual,
        readStoragePowerActorClaims(light_ipld, *asBlake(state_root), false));
    EXPECT_EQ(*asBlake(expected), actual);
  }

}  // namespace fc::codec::cbor::light_reader
