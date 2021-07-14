/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/state/impl/state_tree_impl.hpp"

#include <gtest/gtest.h>

#include "cbor_blake/ipld_any.hpp"
#include "codec/cbor/light_reader/actor.hpp"
#include "codec/cbor/light_reader/hamt_walk.hpp"
#include "primitives/address/address_codec.hpp"
#include "testutil/init_actor.hpp"
#include "vm/actor/codes.hpp"

using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::vm::actor::Actor;
using fc::vm::actor::CodeId;
using fc::vm::state::StateTreeImpl;

const auto kAddressId = Address::makeFromId(13);
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
  EXPECT_OUTCOME_EQ(tree_.tryGet(kAddressId), boost::none);
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
  tree_.txBegin();
  EXPECT_OUTCOME_TRUE_1(tree_.set(kAddressId, kActor));
  tree_.txRevert();
  tree_.txEnd();
  EXPECT_OUTCOME_EQ(tree_.tryGet(kAddressId), boost::none);
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

/**
 * walk visits hamt key-values
 */
TEST_F(StateTreeTest, Walk) {
  using namespace fc;
  adt::Map<vm::actor::Actor, adt::AddressKeyer> map{store_};
  auto head1{setCbor(store_, 3).value()};
  auto code1{vm::actor::code::init0};
  map.set(Address::makeFromId(1), {code1, head1}).value();
  codec::cbor::light_reader::HamtWalk walk{
      std::make_shared<AnyAsCbIpld>(store_),
      *asBlake(map.hamt.flush().value())};
  BytesIn key;
  BytesIn value;
  EXPECT_FALSE(walk.empty());
  EXPECT_TRUE(walk.next(key, value));
  EXPECT_TRUE(walk.empty());
  uint64_t id2{};
  ActorCodeCid code2;
  const CbCid *head2;
  EXPECT_TRUE(codec::cbor::light_reader::readIdAddress(id2, key));
  EXPECT_EQ(id2, 1);
  EXPECT_TRUE(codec::cbor::light_reader::readActor(code2, head2, value));
  EXPECT_EQ(code2, code1);
  EXPECT_EQ(*head2, *asBlake(head1));
  EXPECT_FALSE(walk.next(key, value));
}
