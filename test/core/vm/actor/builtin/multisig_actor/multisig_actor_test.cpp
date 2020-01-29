/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/multisig_actor/multisig_actor.hpp"

#include <gtest/gtest.h>
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor_method.hpp"

using fc::vm::actor::Actor;
using fc::vm::actor::encodeActorParams;
using fc::vm::actor::kCronAddress;
using fc::vm::actor::builtin::ConstructParameteres;
using fc::vm::actor::builtin::MultiSigActor;
using fc::vm::runtime::MockRuntime;

class MultisigActorTest : public ::testing::Test {
 protected:
  Actor actor;
  MockRuntime runtime;
};

/**
 * @given Runtime and multisig actor
 * @when constructor is called with immediate caller different from Init Actor
 * @then error WRONG_CALLER returned
 */
TEST_F(MultisigActorTest, ConstructWrongCaller) {
  ConstructParameteres params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(kCronAddress));

  EXPECT_OUTCOME_ERROR(
      MultiSigActor::WRONG_CALLER,
      MultiSigActor::construct(actor, runtime, encoded_params));
}
