/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/init_actor.hpp"

#include <gtest/gtest.h>
#include "primitives/address/address_codec.hpp"
#include "storage/hamt/hamt.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"

using fc::vm::actor::InitActorState;

/** Init actor state CBOR */
TEST(InitActorTest, ActorState) {
  InitActorState state{"010001020000"_cid, 3};
  expectEncodeAndReencode(state, "82d82a470001000102000003"_unhex);
}

TEST(InitActorTest, AddActor) {
  using fc::primitives::address::Address;
  using fc::storage::hamt::Hamt;
  auto store = std::make_shared<fc::storage::ipfs::InMemoryDatastore>();
  EXPECT_OUTCOME_TRUE(root, Hamt(store).flush());
  InitActorState state{root, 3};
  auto address = Address::makeFromId(0);
  auto expected = Address::makeFromId(state.next_id);
  EXPECT_OUTCOME_EQ(state.addActor(store, address), expected);
  EXPECT_EQ(state.next_id, 4);
  EXPECT_OUTCOME_EQ(
      Hamt(store, state.address_map)
          .getCbor<uint64_t>(fc::primitives::address::encodeToString(address)),
      3);
}
