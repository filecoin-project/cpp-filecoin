/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/impl/invoker_impl.hpp"

#include <gtest/gtest.h>
#include "testutil/cbor.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/cron_actor.hpp"

using fc::vm::VMExitCode;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::MockRuntime;

/// invoker returns error or invokes actor method
TEST(InvokerTest, InvokeCron) {
  using namespace fc::vm::actor;

  auto message = UnsignedMessage{kInitAddress, kInitAddress};
  InvokerImpl invoker;
  MockRuntime runtime;

  EXPECT_OUTCOME_ERROR(
      InvokerImpl::CANT_INVOKE_ACCOUNT_ACTOR,
      invoker.invoke({kAccountCodeCid}, runtime, MethodNumber{0}, {}));
  EXPECT_OUTCOME_ERROR(
      InvokerImpl::NO_CODE_OR_METHOD,
      invoker.invoke({CodeId{kEmptyObjectCid}}, runtime, MethodNumber{0}, {}));
  EXPECT_OUTCOME_ERROR(
      InvokerImpl::NO_CODE_OR_METHOD,
      invoker.invoke({kCronCodeCid}, runtime, MethodNumber{1000}, {}));
  EXPECT_CALL(runtime, getMessage()).WillOnce(testing::Return(message));
  EXPECT_OUTCOME_ERROR(
      cron_actor::WRONG_CALL,
      invoker.invoke(
          {kCronCodeCid}, runtime, cron_actor::kEpochTickMethodNumber, {}));
}

/// decodeActorParams returns error or decoded params
TEST(InvokerTest, DecodeActorParams) {
  using fc::vm::actor::decodeActorParams;

  // 80 is cbor empty list, not int
  EXPECT_OUTCOME_ERROR(fc::vm::actor::DECODE_ACTOR_PARAMS_ERROR,
                       decodeActorParams<int>("80"_unhex));
  EXPECT_OUTCOME_EQ(decodeActorParams<int>("03"_unhex), 3);
}

/// encodeActorParams returns error or encoded params
TEST(InvokerTest, EncodeActorParams) {
  using fc::vm::actor::encodeActorParams;

  EXPECT_OUTCOME_ERROR(fc::vm::actor::ENCODE_ACTOR_PARAMS_ERROR,
                       encodeActorParams(fc::CID()));
  EXPECT_OUTCOME_EQ(encodeActorParams(3), "03"_unhex);
}
