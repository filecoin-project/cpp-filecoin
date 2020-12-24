/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/state/impl/state_tree_impl.hpp"

#include <gtest/gtest.h>
#include "primitives/address/address_codec.hpp"
#include "testutil/init_actor.hpp"

using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::storage::hamt::Hamt;
using fc::storage::hamt::HamtError;
using fc::vm::actor::Actor;
using fc::vm::actor::CodeId;
using fc::vm::state::StateTreeImpl;

auto kAddressId = Address::makeFromId(13);
const Actor kActor{
    CodeId{"010001020001"_cid}, "010001020002"_cid, 3, BigInt(5)};

class StateTreeTest : public ::testing::Test {
 public:
  std::shared_ptr<fc::storage::ipfs::IpfsDatastore> store_{
      std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  StateTreeImpl tree_{store_};
};

/**
 * @given State tree and actor state
 * @when Set actor state in tree
 * @then Actor state in the tree is same
 */
TEST_F(StateTreeTest, Set) {
  EXPECT_OUTCOME_ERROR(HamtError::kNotFound, tree_.get(kAddressId));
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
  EXPECT_OUTCOME_EQ(StateTreeImpl(store_, cid).get(kAddressId), kActor);
}

/**
 * @given Unflushed state tree with actor state
 * @when Revert state tree changes
 * @then Tree doesn't contain actor state
 */
TEST_F(StateTreeTest, SetRevert) {
  EXPECT_OUTCOME_TRUE(root, tree_.flush());
  EXPECT_OUTCOME_TRUE_1(tree_.set(kAddressId, kActor));
  EXPECT_OUTCOME_TRUE_1(tree_.revert(root));
  EXPECT_OUTCOME_ERROR(HamtError::kNotFound, tree_.get(kAddressId));
}

/**
 * @given State tree and actor state
 * @when Register new actor address and state
 * @then Actor state in the tree is same
 */
TEST_F(StateTreeTest, RegisterNewAddressLookupId) {
  auto tree = setupInitActor(nullptr, 13);
  Address address{fc::primitives::address::ActorExecHash{}};
  EXPECT_OUTCOME_EQ(tree->registerNewAddress(address), kAddressId);
  EXPECT_OUTCOME_EQ(tree->lookupId(address), kAddressId);
}
