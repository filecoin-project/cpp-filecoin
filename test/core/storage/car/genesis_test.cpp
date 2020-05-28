/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "primitives/block/block.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"
#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/actor/builtin/init/init_actor.hpp"
#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/miner/types.hpp"
#include "vm/actor/builtin/reward/reward_actor.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"

/**
 * @given genesis file
 * @when decode
 * @then success
 */
TEST(GenesisTest, Decode) {
  auto ipld = std::make_shared<fc::storage::ipfs::InMemoryDatastore>();
  auto input = readFile(resourcePath("genesis.car"));
  EXPECT_OUTCOME_TRUE(roots, fc::storage::car::loadCar(*ipld, input));
  EXPECT_OUTCOME_TRUE(
      block, ipld->getCbor<fc::primitives::block::BlockHeader>(roots[0]));
  fc::adt::Map<fc::vm::actor::Actor, fc::adt::AddressKeyer> state_tree{
      block.parent_state_root};
  state_tree.load(ipld);
  EXPECT_OUTCOME_TRUE_1(state_tree.visit([&](auto &a, auto &actor) {
    // TODO(turuslan): FIL-201
    auto disabled = true;
    if (actor.code == fc::vm::actor::kStorageMinerCodeCid && !disabled) {
      // TODO: update miner actor state
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::miner::MinerActorState>(
              actor.head));
    } else if (actor.code == fc::vm::actor::kStorageMarketCodeCid) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::market::State>(actor.head));
    } else if (actor.code == fc::vm::actor::kAccountCodeCid) {
      EXPECT_OUTCOME_TRUE(head, ipld->get(actor.head));
      if (head.size() != 1 || head[0] != 0x80) {
        EXPECT_OUTCOME_TRUE_1(
            ipld->getCbor<fc::vm::actor::builtin::account::AccountActorState>(
                actor.head));
      }
    } else if (actor.code == fc::vm::actor::kCronCodeCid) {
      // TODO: cron actor state
    } else if (actor.code == fc::vm::actor::kInitCodeCid) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::init::InitActorState>(
              actor.head));
    } else if (actor.code == fc::vm::actor::kRewardActorCodeID && !disabled) {
      // TODO: update reward actor state
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::reward::State>(actor.head));
    } else if (actor.code == fc::vm::actor::kSystemActorCodeID) {
      // TODO: system actor state
    } else if (actor.code == fc::vm::actor::kVerifiedRegistryCode) {
      // TODO: verified registry actor state
    } else if (actor.code == fc::vm::actor::kStoragePowerCodeCid && !disabled) {
      // TODO: update power actor state
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<
              fc::vm::actor::builtin::storage_power::StoragePowerActorState>(
              actor.head));
    }
    return fc::outcome::success();
  }));
}
