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

/** Init actor state CBOR encoding and decoding */
TEST(InitActorTest, InitActorStateCbor) {
  InitActorState init_actor_state{"010001020000"_cid, 3};
  expectEncodeAndReencode(init_actor_state, "82d82a470001000102000003"_unhex);
}

/**
 * @given Init actor state and actor address
 * @when Add actor address
 * @then Actor address is mapped to id
 */
TEST(InitActorTest, AddActor) {
  using fc::primitives::address::Address;
  using fc::storage::hamt::Hamt;
  auto store = std::make_shared<fc::storage::ipfs::InMemoryDatastore>();
  EXPECT_OUTCOME_TRUE(empty_map, Hamt(store).flush());
  InitActorState state{empty_map, 3};
  Address address{fc::primitives::address::TESTNET,
                  fc::primitives::address::ActorExecHash{}};
  auto expected = Address::makeFromId(state.next_id);
  EXPECT_OUTCOME_EQ(state.addActor(store, address), expected);
  EXPECT_EQ(state.next_id, 4);
  EXPECT_OUTCOME_EQ(
      Hamt(store, state.address_map)
          .getCbor<uint64_t>(fc::primitives::address::encodeToString(address)),
      3);
}
