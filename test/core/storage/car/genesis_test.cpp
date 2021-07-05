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
#include "vm/actor/builtin/v0/account/account_actor_state.hpp"
#include "vm/actor/builtin/v0/cron/cron_actor_state.hpp"
#include "vm/actor/builtin/v0/init/init_actor_state.hpp"
#include "vm/actor/builtin/v0/market/market_actor_state.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_state.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/v0/system/system_actor_state.hpp"
#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor_state.hpp"
#include "vm/actor/codes.hpp"

namespace outcome = fc::outcome;

/**
 * @given genesis file
 * @when decode
 * @then success
 */
// TODO: update genesis and actors
TEST(GenesisTest, DISABLED_Decode) {
  auto ipld = std::make_shared<fc::storage::ipfs::InMemoryDatastore>();
  auto nop = [](auto, auto &) { return outcome::success(); };
  auto input = readFile(resourcePath("genesis.car"));
  EXPECT_OUTCOME_TRUE(roots, fc::storage::car::loadCar(*ipld, input));
  EXPECT_OUTCOME_TRUE(
      block, ipld->getCbor<fc::primitives::block::BlockHeader>(roots[0]));
  fc::adt::Map<fc::vm::actor::Actor, fc::adt::AddressKeyer> state_tree{
      block.parent_state_root, ipld};
  auto visit_actors = [&](auto &a, auto &actor) {
    if (actor.code == fc::vm::actor::builtin::v0::kStorageMinerCodeId) {
      // TODO: update genesis
    } else if (actor.code == fc::vm::actor::builtin::v0::kStorageMarketCodeId) {
      EXPECT_OUTCOME_TRUE(
          state,
          ipld->getCbor<fc::vm::actor::builtin::v0::market::MarketActorState>(
              actor.head));
      EXPECT_OUTCOME_TRUE_1(state.proposals.visit(nop));
      EXPECT_OUTCOME_TRUE_1(state.states.visit(nop));
      EXPECT_OUTCOME_TRUE_1(state.escrow_table.visit(nop));
      EXPECT_OUTCOME_TRUE_1(state.locked_table.visit(nop));
      EXPECT_OUTCOME_TRUE_1(state.deals_by_epoch.visit(
          [&](auto, auto set) { return set.visit(nop); }));
    } else if (actor.code == fc::vm::actor::builtin::v0::kAccountCodeId) {
      EXPECT_OUTCOME_TRUE(head, ipld->get(actor.head));
      if (head.size() != 1 || head[0] != 0x80) {
        EXPECT_OUTCOME_TRUE_1(
            ipld->getCbor<
                fc::vm::actor::builtin::v0::account::AccountActorState>(
                actor.head));
      }
    } else if (actor.code == fc::vm::actor::builtin::v0::kCronCodeId) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::v0::cron::CronActorState>(
              actor.head));
    } else if (actor.code == fc::vm::actor::builtin::v0::kInitCodeId) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::v0::init::InitActorState>(
              actor.head));
    } else if (actor.code == fc::vm::actor::builtin::v0::kRewardActorCodeId) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::v0::reward::RewardActorState>(
              actor.head));
    } else if (actor.code == fc::vm::actor::builtin::v0::kSystemActorCodeId) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::v0::system::SystemActorState>(
              actor.head));
    } else if (actor.code
               == fc::vm::actor::builtin::v0::kVerifiedRegistryCodeId) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::v0::verified_registry::
                            VerifiedRegistryActorState>(actor.head));
    } else if (actor.code == fc::vm::actor::builtin::v0::kStorageMarketCodeId) {
      EXPECT_OUTCOME_TRUE(
          state,
          ipld->getCbor<
              fc::vm::actor::builtin::v0::storage_power::PowerActorState>(
              actor.head));
      EXPECT_OUTCOME_TRUE_1(state.claims0.visit(nop));
      EXPECT_OUTCOME_TRUE_1(state.cron_event_queue.visit(
          [&](auto, auto events) { return events.visit(nop); }));
    }
    return outcome::success();
  };
  EXPECT_OUTCOME_TRUE_1(state_tree.visit(visit_actors));
}
