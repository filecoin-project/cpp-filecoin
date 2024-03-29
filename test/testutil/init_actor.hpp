/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "vm/actor/builtin/states/init/init_actor_state.hpp"
#include "vm/actor/codes.hpp"
#include "vm/actor/version.hpp"
#include "vm/state/impl/state_tree_impl.hpp"
#include "vm/toolchain/toolchain.hpp"

/// Sets up init actor state
std::shared_ptr<fc::vm::state::StateTree> setupInitActor(
    std::shared_ptr<fc::vm::state::StateTree> state_tree,
    fc::vm::actor::ActorVersion version,
    uint64_t next_id = 100) {
  if (!state_tree) {
    state_tree = std::make_shared<fc::vm::state::StateTreeImpl>(
        std::make_shared<fc::storage::ipfs::InMemoryDatastore>());
  }
  auto store = state_tree->getStore();
  using namespace fc;
  vm::actor::builtin::states::InitActorStatePtr init_state{version};
  init_state->address_map = {store};
  init_state->next_id = next_id;
  init_state->network_name = "n";
  EXPECT_OUTCOME_TRUE(head, fc::setCbor(store, init_state));

  const auto address_matcher =
      vm::toolchain::Toolchain::createAddressMatcher(version);
  EXPECT_OUTCOME_TRUE_1(
      state_tree->set(fc::vm::actor::kInitAddress,
                      {address_matcher->getInitCodeId(), head, 0, 0}));
  return state_tree;
}
