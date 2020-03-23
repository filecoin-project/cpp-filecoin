/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/weight_calculator_impl.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

using fc::blockchain::weight::WeightCalculatorError;
using fc::blockchain::weight::WeightCalculatorImpl;
using fc::primitives::address::Address;
using fc::primitives::block::BlockHeader;
using fc::primitives::tipset::Tipset;
using fc::storage::ipfs::InMemoryDatastore;
using fc::vm::actor::Actor;
using fc::vm::actor::ActorSubstateCID;
using fc::vm::actor::kStoragePowerAddress;
using fc::vm::actor::kStoragePowerCodeCid;
using fc::vm::actor::builtin::storage_power::StoragePowerActorState;
using fc::vm::state::StateTreeImpl;
using Weight = fc::primitives::BigInt;
using fc::power::Power;

struct Params {
  Weight parent_weight;
  Power network_power;
  size_t block_count;
  Weight expected_weight;
};

fc::outcome::result<Weight> calculateWeight(const Params &params) {
  auto ipld = std::make_shared<InMemoryDatastore>();
  auto some_cid = "010001020001"_cid;
  EXPECT_OUTCOME_TRUE(state_cid,
                      ipld->setCbor(StoragePowerActorState{
                          .total_network_power = params.network_power,
                          .miner_count = {},
                          .escrow_table_cid = some_cid,
                          .cron_event_queue_cid = some_cid,
                          .po_st_detected_fault_miners_cid = some_cid,
                          .claims_cid = some_cid,
                          .num_miners_meeting_min_power = {},
                      }));
  StateTreeImpl state_tree{ipld};
  EXPECT_OUTCOME_TRUE_1(state_tree.set(kStoragePowerAddress,
                                       Actor{
                                           .code = kStoragePowerCodeCid,
                                           .head = ActorSubstateCID{state_cid},
                                           .nonce = {},
                                           .balance = {},
                                       }));
  EXPECT_OUTCOME_TRUE(state_root, state_tree.flush());
  Tipset tipset{{},
                {params.block_count,
                 BlockHeader{
                     Address::makeFromId(0),
                     {},
                     {},
                     {},
                     params.parent_weight,
                     {},
                     state_root,
                     some_cid,
                     some_cid,
                     {},
                     {},
                     {},
                     {},
                 }},
                {}};

  return WeightCalculatorImpl{ipld}.calculateWeight(tipset);
}

struct WeightCalculatorTest : ::testing::TestWithParam<Params> {};

TEST_F(WeightCalculatorTest, ZeroNetworkPower) {
  EXPECT_OUTCOME_ERROR(WeightCalculatorError::NO_NETWORK_POWER,
                       calculateWeight({{}, 0, 1, {}}));
}

TEST_P(WeightCalculatorTest, Success) {
  auto &params = GetParam();
  EXPECT_OUTCOME_EQ(calculateWeight(params), params.expected_weight);
}

INSTANTIATE_TEST_CASE_P(WeightCalculatorTestCases,
                        WeightCalculatorTest,
                        ::testing::Values(Params{100, 200, 1, 2071},
                                          Params{100, 200, 2, 2250},
                                          Params{100, 200, 3, 2429},
                                          Params{100, 200, 4, 2608},
                                          Params{100, 200, 5, 2788},
                                          Params{200, 200, 3, 2529},
                                          Params{100, 2000, 3, 3428}));
