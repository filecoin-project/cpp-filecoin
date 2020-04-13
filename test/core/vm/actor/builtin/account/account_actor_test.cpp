/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/account/account_actor.hpp"

#include "testutil/init_actor.hpp"

using fc::primitives::address::Address;
using fc::vm::VMExitCode;
using fc::vm::actor::builtin::account::AccountActor;
using fc::vm::actor::builtin::account::AccountActorState;
using fc::vm::state::StateTree;

/// Account actor state CBOR encoding and decoding
TEST(AccountActorTest, AccountActorStateCbor) {
  AccountActorState state{Address::makeFromId(3)};
  expectEncodeAndReencode(state, "81420003"_unhex);
}

/**
 * @given Empty state tree and actor
 * @when Create account actor with BLS address @and resolve BLS address from
 * assigned ID address
 * @then Returned BLS address is equal to original
 */
TEST(InitActorTest, CreateResolve) {
  auto state_tree = setupInitActor(nullptr);
  auto addr1 = Address::makeFromId(3);
  Address addr2{
      fc::primitives::address::Network::TESTNET,
      fc::primitives::address::BLSPublicKeyHash{
          "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob48}};
  fc::vm::actor::Actor actor{
      fc::vm::actor::kCronCodeCid,
      fc::vm::actor::ActorSubstateCID{"010001020002"_cid},
      0,
      0};

  EXPECT_OUTCOME_ERROR(VMExitCode::ACCOUNT_ACTOR_CREATE_WRONG_ADDRESS_TYPE,
                       AccountActor::create(state_tree, addr1));

  EXPECT_OUTCOME_TRUE_1(state_tree->set(addr1, actor));

  actor.code = fc::vm::actor::kAccountCodeCid;
  EXPECT_OUTCOME_TRUE(actor2, AccountActor::create(state_tree, addr2));
  EXPECT_EQ(actor2.code, fc::vm::actor::kAccountCodeCid);
  EXPECT_OUTCOME_TRUE_1(state_tree->lookupId(addr2));
}
