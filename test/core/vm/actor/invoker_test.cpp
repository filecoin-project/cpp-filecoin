/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/impl/invoker_impl.hpp"

#include <gtest/gtest.h>
#include "testutil/cbor.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/codes.hpp"
#include "vm/runtime/env.hpp"

namespace fc::vm::actor {
  using builtin::v0::kCronCodeId;
  using message::UnsignedMessage;
  using primitives::ChainEpoch;
  using runtime::Env;
  using runtime::Execution;
  using runtime::MockRuntime;

  /// invoker returns error or invokes actor method
  TEST(InvokerTest, InvokeCron) {
    auto message =
        UnsignedMessage{kInitAddress, kInitAddress, {}, {}, {}, {}, {}, {}};
    auto runtime = std::make_shared<MockRuntime>();
    auto ts{std::make_shared<primitives::tipset::Tipset>()};
    ts->blks.emplace_back();
    auto execution{std::make_shared<Execution>()};
    execution->env = std::make_shared<Env>();

    InvokerImpl invoker;

    // Error on wrong actor
    EXPECT_CALL(*runtime, execution())
        .WillRepeatedly(testing::Return(execution));
    EXPECT_CALL(*runtime, getMessage()).WillOnce(testing::Return(message));
    EXPECT_CALL(*runtime, getCurrentEpoch())
        .WillOnce(testing::Return(ChainEpoch{}));
    EXPECT_CALL(*runtime, getNetworkVersion())
        .WillOnce(testing::Return(NetworkVersion{}));
    EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrIllegalActor,
                         invoker.invoke({CodeId{kEmptyObjectCid}}, runtime));

    // Error on wrong method
    message.method = MethodNumber{1000};
    EXPECT_CALL(*runtime, getMessage()).WillOnce(testing::Return(message));
    EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrInvalidMethod,
                         invoker.invoke({kCronCodeId}, runtime));
  }

  /// decodeActorParams returns error or decoded params
  TEST(InvokerTest, DecodeActorParams) {
    using fc::vm::actor::decodeActorParams;

    // 80 is cbor empty list, not int
    EXPECT_OUTCOME_ERROR(
        VMExitCode::kOldErrActorFailure,
        decodeActorParams<int>(MethodParams{"80"_unhex}, false));
    EXPECT_OUTCOME_ERROR(
        VMExitCode::kErrSerialization,
        decodeActorParams<int>(MethodParams{"80"_unhex}, true));
    EXPECT_OUTCOME_EQ(decodeActorParams<int>(MethodParams{"03"_unhex}, false),
                      3);
  }

  /// encodeActorParams returns error or encoded params
  TEST(InvokerTest, EncodeActorParams) {
    using fc::vm::actor::encodeActorParams;

    EXPECT_OUTCOME_ERROR(VMExitCode::kErrSerialization,
                         encodeActorParams(fc::CID()));
    EXPECT_OUTCOME_EQ(encodeActorParams(3), MethodParams{"03"_unhex});
  }
}  // namespace fc::vm::actor
