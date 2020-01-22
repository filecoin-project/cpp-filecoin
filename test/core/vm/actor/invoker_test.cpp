/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/invoker.hpp"

#include <gtest/gtest.h>
#include "core/vm/runtime/runtime_mock.hpp"
#include "testutil/cbor.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/cron_actor.hpp"

using fc::vm::VMExitCode;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::MockRuntime;

/// invoker returns error or invokes actor method
TEST(InvokerTest, InvokeCron) {
  using namespace fc::vm::actor;

  auto message = std::make_shared<UnsignedMessage>(
      UnsignedMessage{kInitAddress, kInitAddress});
  Invoker invoker;
  auto runtime = std::make_shared<MockRuntime>();

  EXPECT_OUTCOME_ERROR(
      Invoker::CANT_INVOKE_ACCOUNT_ACTOR,
      invoker.invoke({kAccountCodeCid}, runtime, MethodNumber{0}, {}));
  EXPECT_OUTCOME_ERROR(
      Invoker::NO_CODE_OR_METHOD,
      invoker.invoke({CodeId{kEmptyObjectCid}}, runtime, MethodNumber{0}, {}));
  EXPECT_OUTCOME_ERROR(
      Invoker::NO_CODE_OR_METHOD,
      invoker.invoke({kCronCodeCid}, runtime, MethodNumber{1000}, {}));
  EXPECT_CALL(*runtime, getMessage()).WillRepeatedly(testing::Return(message));
  EXPECT_OUTCOME_ERROR(
      CronActor::WRONG_CALL,
      invoker.invoke({kCronCodeCid}, runtime, MethodNumber{2}, {}));
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
