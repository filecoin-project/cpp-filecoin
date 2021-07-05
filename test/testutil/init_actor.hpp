/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "vm/actor/builtin/v0/init/init_actor_state.hpp"
#include "vm/actor/codes.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

/// Sets up init actor state
std::shared_ptr<fc::vm::state::StateTree> setupInitActor(
    std::shared_ptr<fc::vm::state::StateTree> state_tree,
    uint64_t next_id = 100) {
  if (!state_tree) {
    state_tree = std::make_shared<fc::vm::state::StateTreeImpl>(
        std::make_shared<fc::storage::ipfs::InMemoryDatastore>());
  }
  auto store = state_tree->getStore();
  fc::vm::actor::builtin::v0::init::InitActorState init_state;
  init_state.address_map_0 = {store};
  init_state.next_id = next_id;
  init_state.network_name = "n";
  EXPECT_OUTCOME_TRUE(head, store->setCbor(init_state));
  EXPECT_OUTCOME_TRUE_1(
      state_tree->set(fc::vm::actor::kInitAddress,
                      {fc::vm::actor::builtin::v0::kInitCodeId, head, 0, 0}));
  return state_tree;
}
