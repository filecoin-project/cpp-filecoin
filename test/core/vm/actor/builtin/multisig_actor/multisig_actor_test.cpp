/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/multisig_actor/multisig_actor.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/mocks/storage/ipfs/ipfs_datastore_mock.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor_method.hpp"

using fc::CID;
using fc::primitives::ChainEpoch;
using fc::storage::ipfs::MockIpfsDatastore;
using fc::vm::actor::Actor;
using fc::vm::actor::DECODE_ACTOR_PARAMS_ERROR;
using fc::vm::actor::encodeActorParams;
using fc::vm::actor::kCronAddress;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::MethodParams;
using fc::vm::actor::builtin::ConstructParameteres;
using fc::vm::actor::builtin::MultiSigActor;
using fc::vm::runtime::MockRuntime;
using ::testing::_;

class MultisigActorTest : public ::testing::Test {
 protected:
  Actor actor;
  MockRuntime runtime;
  std::shared_ptr<MockIpfsDatastore> datastore =
      std::make_shared<MockIpfsDatastore>();
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

/**
 * @given Runtime and multisig actor
 * @when constructor is called with wrong encoded constructor params
 * @then serialization error returned
 */
TEST_F(MultisigActorTest, ConstructWrongParams) {
  MethodParams wrong_params{"DEAD"_unhex};

  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(kInitAddress));

  EXPECT_OUTCOME_ERROR(DECODE_ACTOR_PARAMS_ERROR,
                       MultiSigActor::construct(actor, runtime, wrong_params));
}

/**
 * @given Runtime and multisig actor
 * @when constructor is called with correct parameters
 * @then success returned and state is commited to storage
 */
TEST_F(MultisigActorTest, ConstrucCorrect) {
  ConstructParameteres params{};
  CID cid{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(kInitAddress));
  EXPECT_CALL(runtime, getCurrentEpoch())
      .WillOnce(testing::Return(ChainEpoch{42}));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, set(_, _))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_TRUE_1(
      MultiSigActor::construct(actor, runtime, encoded_params));
}
