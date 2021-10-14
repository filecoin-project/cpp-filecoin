/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/account/account_actor.hpp"
#include "vm/actor/builtin/states/account/account_actor_state.hpp"

#include <gtest/gtest.h>

#include "testutil/cbor.hpp"
#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"

namespace fc::vm::actor::builtin::v2::account {
  using primitives::address::Address;
  using states::AccountActorState;
  using states::AccountActorStatePtr;
  using testutil::vm::actor::builtin::ActorTestFixture;

  struct AccountActorTest : public ActorTestFixture<AccountActorState> {
    void SetUp() override {
      ActorTestFixture<AccountActorState>::SetUp();
      actor_version = ActorVersion::kVersion2;
      ipld->actor_version = actor_version;
      state = AccountActorStatePtr{actor_version};
    }

    Address address{Address::makeSecp256k1(
        {0xFD, 0x1D, 0x0F, 0x4D, 0xFC, 0xD7, 0xE9, 0x9A, 0xFC, 0xB9,
         0x9A, 0x83, 0x26, 0xB7, 0xDC, 0x45, 0x9D, 0x32, 0xC6, 0x28})};
  };

  /// Account actor state CBOR encoding and decoding
  TEST_F(AccountActorTest, AccountActorStateCbor) {
    state->address = Address::makeFromId(3);
    expectEncodeAndReencode(state, "81420003"_unhex);
  }

  TEST_F(AccountActorTest, ConstructWrongCaller) {
    callerIs(kInitAddress);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         Construct::call(runtime, {}));
  }

  TEST_F(AccountActorTest, ConstructNotKeyAddress) {
    callerIs(kSystemActorAddress);

    const auto not_key_address = Address::makeFromId(5);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         Construct::call(runtime, not_key_address));
  }

  TEST_F(AccountActorTest, ConstructSuccess) {
    callerIs(kSystemActorAddress);

    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, address));

    EXPECT_EQ(state->address, address);
  }

  TEST_F(AccountActorTest, PubkeyAddressSuccess) {
    const auto address = Address::makeFromId(5);
    state->address = address;

    EXPECT_OUTCOME_TRUE(result, PubkeyAddress::call(runtime, {}));

    EXPECT_EQ(result, address);
  }

}  // namespace fc::vm::actor::builtin::v2::account
