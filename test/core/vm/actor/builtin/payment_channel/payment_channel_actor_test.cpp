/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/payment_channel/payment_channel_actor.hpp"

#include "gtest/gtest.h"
#include "primitives/address/address.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/storage/ipfs/ipfs_datastore_mock.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/payment_channel/payment_channel_actor_state.hpp"

using fc::primitives::address::Address;
using fc::storage::ipfs::MockIpfsDatastore;
using fc::vm::VMExitCode;
using fc::vm::actor::Actor;
using fc::vm::actor::encodeActorParams;
using fc::vm::actor::kAccountCodeCid;
using fc::vm::actor::kCronAddress;
using fc::vm::actor::kCronCodeCid;
using fc::vm::actor::MethodParams;
using fc::vm::actor::builtin::payment_channel::ConstructParameteres;
using fc::vm::actor::builtin::payment_channel::PaymentChannelActor;
using fc::vm::actor::builtin::payment_channel::PaymentChannelActorState;
using fc::vm::runtime::InvocationOutput;
using fc::vm::runtime::MockRuntime;
using ::testing::_;
using ::testing::Eq;

/**
 * Match encoded PaymentChannelState is expected one
 * Used for IpldStore set state.
 */
MATCHER_P(PaymentChannelStateMatcher, expected, "Match PaymentChannelState") {
  PaymentChannelActorState actual =
      fc::codec::cbor::decode<PaymentChannelActorState>(arg).value();
  return actual.from == expected.from && actual.to == expected.to
         && actual.to_send == expected.to_send
         && actual.settling_at == expected.settling_at
         && actual.min_settling_height == expected.min_settling_height
         && actual.lanes == expected.lanes;
}

class PaymentChannelActorTest : public ::testing::Test {
 protected:
  Actor actor{kAccountCodeCid};
  Address caller_address = Address::makeFromId(200);
  Address to_address = Address::makeFromId(42);  // arbitrary address
  MockRuntime runtime;
  std::shared_ptr<MockIpfsDatastore> datastore =
      std::make_shared<MockIpfsDatastore>();
};

/**
 * @given Runtime
 * @when constructor is called with immediate caller different from AccountActor
 * @then error WRONG_CALLER returned
 */
TEST_F(PaymentChannelActorTest, ConstructWrongCaller) {
  actor.code = kCronCodeCid;  // not an account code id
  ConstructParameteres params{to_address};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_WRONG_CALLER,
      PaymentChannelActor::construct(actor, runtime, encoded_params));
}

/**
 * @given Runtime
 * @when constructor is called with malformed parameters
 * @then error returned
 */
TEST_F(PaymentChannelActorTest, ConstructWrongParams) {
  MethodParams wrong_params{"DEAD"_unhex};

  EXPECT_OUTCOME_ERROR(
      VMExitCode::DECODE_ACTOR_PARAMS_ERROR,
      PaymentChannelActor::construct(actor, runtime, wrong_params));
}

/**
 * @given Runtime
 * @when constructor is called with target address Protocol not ID
 * @then error WRONG_ARGUMENT returned
 */
TEST_F(PaymentChannelActorTest, ConstructWrongTargetAddressProtocol) {
  Address wrong_address = Address::makeActorExec("DEAD"_unhex);
  ConstructParameteres params{wrong_address};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannelActor::construct(actor, runtime, encoded_params));
}

/**
 * @given Runtime
 * @when constructor is called with target not Account Code
 * @then error WRONG_ARGUMENT returned
 */
TEST_F(PaymentChannelActorTest, ConstructWrongTarget) {
  ConstructParameteres params{to_address};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getActorCodeID(Eq(to_address)))
      .WillOnce(testing::Return(fc::outcome::success(kCronCodeCid)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannelActor::construct(actor, runtime, encoded_params));
}

/**
 * @given Runtime
 * @when constructor is called
 * @then State is constructed and committed
 */
TEST_F(PaymentChannelActorTest, ConstructSuccess) {
  ConstructParameteres params{to_address};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  PaymentChannelActorState expected_state{
      caller_address, to_address, 0, 0, 0, {}};

  EXPECT_CALL(runtime, getActorCodeID(Eq(to_address)))
      .WillOnce(testing::Return(fc::outcome::success(kAccountCodeCid)));
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(caller_address));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, set(_, PaymentChannelStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_EQ(
      PaymentChannelActor::construct(actor, runtime, encoded_params),
      InvocationOutput{});
}

/**
 * @given Runtime
 * @when UpdateChannelState is called with caller_address not in state
 * @then error WRONG_CALLER returned
 */
TEST_F(PaymentChannelActorTest, UpdateChannelStateWrongTarget) {
  Address wrong_caller_address = Address::makeFromId(404);
  ConstructParameteres params{to_address};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  PaymentChannelActorState actor_state{
      caller_address, to_address, 0, 0, 0, {}};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(wrong_caller_address));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_WRONG_CALLER,
      PaymentChannelActor::updateChannelState(actor, runtime, encoded_params));
}
