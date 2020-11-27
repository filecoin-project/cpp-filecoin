#ifndef CPP_FILECOIN_TEST_TESTUTIL_INIT_ACTOR_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_INIT_ACTOR_HPP

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/init/init_actor.hpp"
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
  fc::vm::actor::builtin::v0::init::InitActorState init_state{
      {store}, next_id, "n"};
  EXPECT_OUTCOME_TRUE(head, store->setCbor(init_state));
  EXPECT_OUTCOME_TRUE_1(
      state_tree->set(fc::vm::actor::kInitAddress,
                      {fc::vm::actor::builtin::v0::kInitCodeCid, head, 0, 0}));
  return state_tree;
}

#endif  // CPP_FILECOIN_TEST_TESTUTIL_INIT_ACTOR_HPP
