/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/account/account_actor.hpp"

#include "testutil/init_actor.hpp"

using fc::primitives::address::Address;
using fc::vm::VMExitCode;
using fc::vm::actor::builtin::account::AccountActorState;
using fc::vm::state::StateTree;

/// Account actor state CBOR encoding and decoding
TEST(AccountActorTest, AccountActorStateCbor) {
  AccountActorState state{Address::makeFromId(3)};
  expectEncodeAndReencode(state, "81420003"_unhex);
}
