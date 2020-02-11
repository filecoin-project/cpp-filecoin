#ifndef CPP_FILECOIN_TEST_TESTUTIL_INIT_ACTOR_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_INIT_ACTOR_HPP

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "vm/actor/builtin/init/init_actor.hpp"
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
  EXPECT_OUTCOME_TRUE(empty_map, fc::storage::hamt::Hamt(store).flush());
  EXPECT_OUTCOME_TRUE(
      state,
      store->setCbor(
          fc::vm::actor::builtin::init::InitActorState{empty_map, next_id}));
  EXPECT_OUTCOME_TRUE_1(state_tree->set(fc::vm::actor::kInitAddress,
                                        {fc::vm::actor::kInitCodeCid,
                                         fc::vm::actor::ActorSubstateCID{state},
                                         0,
                                         0}));
  return state_tree;
}

#endif  // CPP_FILECOIN_TEST_TESTUTIL_INIT_ACTOR_HPP
