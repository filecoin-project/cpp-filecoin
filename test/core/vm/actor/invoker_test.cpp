/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/impl/invoker_impl.hpp"

#include <gtest/gtest.h>
#include "testutil/cbor.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/cron/cron_actor.hpp"

using fc::vm::actor::builtin::v0::kCronCodeCid;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::MockRuntime;

namespace fc::vm::actor {

  /// invoker returns error or invokes actor method
  TEST(InvokerTest, InvokeCron) {
    auto message =
        UnsignedMessage{kInitAddress, kInitAddress, {}, {}, {}, {}, {}, {}};
    InvokerImpl invoker;
    auto runtime = std::make_shared<MockRuntime>();

    // Error on wrong actor
    EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrorIllegalActor,
                         invoker.invoke({CodeId{kEmptyObjectCid}}, runtime));

    // Error on wrong method
    message.method = MethodNumber{1000};
    EXPECT_CALL(*runtime, getMessage()).WillOnce(testing::Return(message));
    EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrInvalidMethod,
                         invoker.invoke({kCronCodeCid}, runtime));
  }

  /// decodeActorParams returns error or decoded params
  TEST(InvokerTest, DecodeActorParams) {
    using fc::vm::actor::decodeActorParams;

    // 80 is cbor empty list, not int
    EXPECT_OUTCOME_ERROR(VMExitCode::kDecodeActorParamsError,
                         decodeActorParams<int>(MethodParams{"80"_unhex}));
    EXPECT_OUTCOME_EQ(decodeActorParams<int>(MethodParams{"03"_unhex}), 3);
  }

  /// encodeActorParams returns error or encoded params
  TEST(InvokerTest, EncodeActorParams) {
    using fc::vm::actor::encodeActorParams;

    EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrInvalidParameters,
                         encodeActorParams(fc::CID()));
    EXPECT_OUTCOME_EQ(encodeActorParams(3), MethodParams{"03"_unhex});
  }
}  // namespace fc::vm::actor
