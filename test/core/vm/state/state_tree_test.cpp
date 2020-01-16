/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/state/state_tree.hpp"

#include <gtest/gtest.h>
#include "primitives/address/address_codec.hpp"
#include "storage/hamt/hamt.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "vm/actor/init_actor.hpp"

using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::storage::hamt::Hamt;
using fc::storage::hamt::HamtError;
using fc::vm::actor::Actor;
using fc::vm::actor::ActorSubstateCID;
using fc::vm::actor::CodeId;
using fc::vm::state::StateTree;

auto kAddressId = Address::makeFromId(13);
const Actor kActor{CodeId{"010001020001"_cid},
                   ActorSubstateCID{"010001020002"_cid},
                   3,
                   BigInt(5)};

class StateTreeTest : public ::testing::Test {
 public:
  std::shared_ptr<fc::storage::ipfs::IpfsDatastore> store_{
      std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  StateTree tree_{store_};
};

/**
 * @given State tree and actor state
 * @when Set actor state in tree
 * @then Actor state in the tree is same
 */
TEST_F(StateTreeTest, Set) {
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, tree_.get(kAddressId));
  EXPECT_OUTCOME_TRUE_1(tree_.set(kAddressId, kActor));
  EXPECT_OUTCOME_EQ(tree_.get(kAddressId), kActor);
}

/**
 * @given Unflushed state tree with actor state
 * @when Flush state tree changes
 * @then Tree contains actor state
 */
TEST_F(StateTreeTest, SetFlush) {
  EXPECT_OUTCOME_TRUE_1(tree_.set(kAddressId, kActor));
  EXPECT_OUTCOME_TRUE(cid, tree_.flush());
  EXPECT_OUTCOME_EQ(tree_.get(kAddressId), kActor);
  EXPECT_OUTCOME_EQ(StateTree(store_, cid).get(kAddressId), kActor);
}

/**
 * @given Unflushed state tree with actor state
 * @when Revert state tree changes
 * @then Tree doesn't contain actor state
 */
TEST_F(StateTreeTest, SetRevert) {
  EXPECT_OUTCOME_TRUE_1(tree_.set(kAddressId, kActor));
  EXPECT_OUTCOME_TRUE_1(tree_.revert());
  EXPECT_OUTCOME_ERROR(HamtError::NOT_FOUND, tree_.get(kAddressId));
}

/**
 * @given State tree and actor state
 * @when Register new actor address and state
 * @then Actor state in the tree is same
 */
TEST_F(StateTreeTest, RegisterNewAddressLookupId) {
  using fc::vm::actor::InitActorState;
  using fc::vm::actor::kInitAddress;
  EXPECT_OUTCOME_TRUE(empty_map, Hamt(store_).flush());
  InitActorState init_actor_state{empty_map, 13};
  EXPECT_OUTCOME_TRUE(init_actor_state_cid, store_->setCbor(init_actor_state));
  Actor init_actor{CodeId{"010001020000"_cid},
                   ActorSubstateCID{init_actor_state_cid},
                   {},
                   {}};
  EXPECT_OUTCOME_TRUE_1(tree_.set(kInitAddress, init_actor));
  Address address{fc::primitives::address::TESTNET,
                  fc::primitives::address::ActorExecHash{}};
  EXPECT_OUTCOME_EQ(tree_.registerNewAddress(address, kActor), kAddressId);
  EXPECT_OUTCOME_EQ(tree_.lookupId(address), kAddressId);
  EXPECT_OUTCOME_EQ(tree_.get(address), kActor);
  EXPECT_OUTCOME_EQ(tree_.get(kAddressId), kActor);
}
