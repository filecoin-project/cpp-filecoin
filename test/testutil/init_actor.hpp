/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "vm/actor/builtin/states/init_actor_state_raw.hpp"
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
  using namespace fc;
  vm::actor::InitActorStateRaw init_state;
  init_state.address_map = {store, 0, false};
  init_state.next_id = next_id;
  init_state.network_name = "n";
  EXPECT_OUTCOME_TRUE(head, store->setCbor(init_state));
  // TODO(turuslan): remove after PR#415
  CID code{
      CID::Version::V1,
      libp2p::multi::MulticodecType::Code::RAW,
      libp2p::multi::Multihash::create(
          libp2p::multi::HashType::identity,
          common::span::cbytes(vm::actor::code::init0))
          .value(),
  };
  EXPECT_OUTCOME_TRUE_1(
      state_tree->set(fc::vm::actor::kInitAddress, {code, head, 0, 0}));
  return state_tree;
}
