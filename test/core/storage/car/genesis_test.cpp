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
#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/cron/cron_actor.hpp"
#include "vm/actor/builtin/v0/init/init_actor.hpp"
#include "vm/actor/builtin/v0/market/actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_state.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor.hpp"

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
    if (actor.code == fc::vm::actor::builtin::v0::kStorageMinerCodeCid) {
      // TODO: update genesis
    } else if (actor.code
               == fc::vm::actor::builtin::v0::kStorageMarketCodeCid) {
      EXPECT_OUTCOME_TRUE(
          state,
          ipld->getCbor<fc::vm::actor::builtin::v0::market::State>(actor.head));
      EXPECT_OUTCOME_TRUE_1(state.proposals.visit(nop));
      EXPECT_OUTCOME_TRUE_1(state.states.visit(nop));
      EXPECT_OUTCOME_TRUE_1(state.escrow_table.visit(nop));
      EXPECT_OUTCOME_TRUE_1(state.locked_table.visit(nop));
      EXPECT_OUTCOME_TRUE_1(state.deals_by_epoch.visit(
          [&](auto, auto set) { return set.visit(nop); }));
    } else if (actor.code == fc::vm::actor::builtin::v0::kAccountCodeCid) {
      EXPECT_OUTCOME_TRUE(head, ipld->get(actor.head));
      if (head.size() != 1 || head[0] != 0x80) {
        EXPECT_OUTCOME_TRUE_1(
            ipld->getCbor<
                fc::vm::actor::builtin::v0::account::AccountActorState>(
                actor.head));
      }
    } else if (actor.code == fc::vm::actor::builtin::v0::kCronCodeCid) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::v0::cron::State>(actor.head));
    } else if (actor.code == fc::vm::actor::builtin::v0::kInitCodeCid) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::v0::init::InitActorState>(
              actor.head));
    } else if (actor.code == fc::vm::actor::builtin::v0::kRewardActorCodeID) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::v0::reward::State>(actor.head));
    } else if (actor.code == fc::vm::actor::builtin::v0::kSystemActorCodeID) {
      // TODO: system actor state
    } else if (actor.code
               == fc::vm::actor::builtin::v0::kVerifiedRegistryCode) {
      EXPECT_OUTCOME_TRUE_1(
          ipld->getCbor<fc::vm::actor::builtin::v0::verified_registry::State>(
              actor.head));
    } else if (actor.code == fc::vm::actor::builtin::v0::kStoragePowerCodeCid) {
      EXPECT_OUTCOME_TRUE(
          state,
          ipld->getCbor<fc::vm::actor::builtin::v0::storage_power::State>(
              actor.head));
      EXPECT_OUTCOME_TRUE_1(state.claims.visit(nop));
      EXPECT_OUTCOME_TRUE_1(state.cron_event_queue.visit(
          [&](auto, auto events) { return events.visit(nop); }));
    }
    return outcome::success();
  };
  EXPECT_OUTCOME_TRUE_1(state_tree.visit(visit_actors));
}
