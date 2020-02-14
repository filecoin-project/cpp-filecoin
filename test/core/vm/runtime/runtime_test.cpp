/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/runtime_impl.hpp"

#include <gtest/gtest.h>
#include <vm/runtime/runtime_error.hpp>

#include "crypto/randomness/randomness_types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"
#include "testutil/cbor.hpp"
#include "testutil/mocks/crypto/randomness/randomness_provider_mock.hpp"
#include "testutil/mocks/storage/ipfs/ipfs_datastore_mock.hpp"
#include "testutil/mocks/vm/actor/invoker_mock.hpp"
#include "testutil/mocks/vm/indices/indices_mock.hpp"
#include "testutil/mocks/vm/state/state_tree_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/message/message.hpp"

using fc::crypto::randomness::ChainEpoch;
using fc::crypto::randomness::MockRandomnessProvider;
using fc::crypto::randomness::RandomnessProvider;
using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::storage::hamt::HamtError;
using fc::storage::ipfs::IpfsDatastore;
using fc::storage::ipfs::MockIpfsDatastore;
using fc::vm::actor::Actor;
using fc::vm::actor::ActorSubstateCID;
using fc::vm::actor::CodeId;
using fc::vm::actor::Invoker;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::MethodNumber;
using fc::vm::actor::MethodParams;
using fc::vm::actor::MockInvoker;
using fc::vm::indices::Indices;
using fc::vm::indices::MockIndices;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::Env;
using fc::vm::runtime::InvocationOutput;
using fc::vm::runtime::Runtime;
using fc::vm::runtime::RuntimeError;
using fc::vm::runtime::RuntimeImpl;
using fc::vm::state::MockStateTree;
using fc::vm::state::StateTree;
using ::testing::_;
using ::testing::Eq;

class RuntimeTest : public ::testing::Test {
 protected:
  Address message_to{fc::primitives::address::TESTNET, 123};
  Address message_from{fc::primitives::address::TESTNET, 345};
  std::shared_ptr<MockRandomnessProvider> randomness_provider_ =
      std::make_shared<MockRandomnessProvider>();
  std::shared_ptr<MockIpfsDatastore> datastore_ =
      std::make_shared<MockIpfsDatastore>();
  std::shared_ptr<MockStateTree> state_tree_ =
      std::make_shared<MockStateTree>();
  std::shared_ptr<MockIndices> indices_ = std::make_shared<MockIndices>();
  std::shared_ptr<MockInvoker> invoker_ = std::make_shared<MockInvoker>();
  UnsignedMessage message_{message_to, message_from};
  ChainEpoch chain_epoch_{0};
  Address immediate_caller_{fc::primitives::address::TESTNET, 1};
  Address block_miner_{};
  BigInt gas_available_{100};
  BigInt gas_used_{0};

  std::shared_ptr<Runtime> runtime_ =
      std::make_shared<RuntimeImpl>(std::make_shared<Env>(randomness_provider_,
                                                          state_tree_,
                                                          indices_,
                                                          invoker_,
                                                          chain_epoch_,
                                                          block_miner_),
                                    message_,
                                    immediate_caller_,
                                    gas_available_,
                                    gas_used_,
                                    ActorSubstateCID{"010001020001"_cid});
};

/**
 * @given Runtime with StateTree with Actor with balance and address
 * @when getBalance is called with address
 * @then balance is returned
 */
TEST_F(RuntimeTest, getBalanceCorrect) {
  Address address{kInitAddress};
  BigInt balance{123};
  Actor actor{CodeId{}, ActorSubstateCID{}, 0, balance};

  EXPECT_CALL(*state_tree_, get(Eq(address)))
      .WillOnce(testing::Return(fc::outcome::success(actor)));

  EXPECT_OUTCOME_EQ(runtime_->getBalance(address), balance);
}

/**
 * @given Runtime with StateTree without Actor with Address
 * @when getBalance is called with Address
 * @then Zero amount returned
 */
TEST_F(RuntimeTest, getBalanceActorNotFound) {
  Address not_found_address{kInitAddress};

  EXPECT_CALL(*state_tree_, get(Eq(not_found_address)))
      .WillOnce(testing::Return(fc::outcome::failure(HamtError::NOT_FOUND)));

  EXPECT_OUTCOME_EQ(runtime_->getBalance(not_found_address), BigInt{0});
}

/**
 * @given Runtime with StateTree without Actor with Address and State Tree with
 * incorrect state
 * @when getBalance is called with Address
 * @then Error propagated to caller
 */
TEST_F(RuntimeTest, getBalanceError) {
  Address not_found_address{kInitAddress};

  EXPECT_CALL(*state_tree_, get(Eq(not_found_address)))
      .WillOnce(testing::Return(fc::outcome::failure(HamtError::MAX_DEPTH)));

  EXPECT_OUTCOME_ERROR(HamtError::MAX_DEPTH,
                       runtime_->getBalance(not_found_address));
}

/**
 * @given Runtime with immediate_caller with InitCodeCid and CodeId and new
 * Address
 * @when createActor is called with CodeId and Address
 * @then Actor is created and success returned
 */
TEST_F(RuntimeTest, createActorValid) {
  Actor immediate_caller_actor{
      fc::vm::actor::kInitCodeCid, ActorSubstateCID{}, 0, 0};
  CodeId new_code{fc::vm::actor::kEmptyObjectCid};
  Address new_address{fc::primitives::address::TESTNET, 2};
  Actor actor{new_code, ActorSubstateCID{fc::vm::actor::kEmptyObjectCid}, 0, 0};

  EXPECT_CALL(*state_tree_, get(Eq(immediate_caller_)))
      .WillOnce(testing::Return(fc::outcome::success(immediate_caller_actor)));
  EXPECT_CALL(*state_tree_, set(Eq(new_address), Eq(actor)))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_TRUE_1(runtime_->createActor(new_address, actor));
}

/**
 * @given Runtime with immediate_caller is not InitActor and CodeId and new
 * Address
 * @when createActor is called with CodeId and Address
 * @then error CREATE_ACTOR_OPERATION_NOT_PERMITTED is returned
 */
TEST_F(RuntimeTest, createActorNotPermitted) {
  Actor immediate_caller_actor{
      fc::vm::actor::kAccountCodeCid, ActorSubstateCID{}, 0, 0};
  CodeId new_code{fc::vm::actor::kEmptyObjectCid};
  Address new_address{fc::primitives::address::TESTNET, 2};

  EXPECT_CALL(*state_tree_, get(Eq(immediate_caller_)))
      .WillOnce(testing::Return(fc::outcome::success(immediate_caller_actor)));

  EXPECT_OUTCOME_ERROR(RuntimeError::CREATE_ACTOR_OPERATION_NOT_PERMITTED,
                       runtime_->createActor(new_address, {}));
}

/**
 * // TODO(a.chernyshov) FIL-139 - this method is not described in specification
 * @given Runtime withfrom actor with funds enough for send
 * @when send() is called
 * @then to_actor successfully called
 */
TEST_F(RuntimeTest, send) {
  Address to_address{fc::primitives::address::TESTNET, 345};
  MethodNumber method{123};
  MethodParams params{};
  BigInt amount{};

  Actor from_actor{fc::vm::actor::kInitCodeCid, ActorSubstateCID{}, 0, 0};
  Actor to_actor{fc::vm::actor::kAccountCodeCid, ActorSubstateCID{}, 0, 0};
  Actor miner_actor{
      fc::vm::actor::kStorageMinerCodeCid, ActorSubstateCID{}, 0, 0};
  InvocationOutput res{};

  EXPECT_CALL(*state_tree_, flush())
      .Times(2)
      .WillRepeatedly(testing::Return(fc::outcome::success()));
  EXPECT_CALL(*state_tree_, get(Eq(message_.to)))
      .WillOnce(testing::Return(fc::outcome::success(from_actor)));
  EXPECT_CALL(*state_tree_, get(Eq(to_address)))
      .WillOnce(testing::Return(fc::outcome::success(to_actor)));
  EXPECT_CALL(*invoker_, invoke(Eq(to_actor), _, Eq(method), Eq(params)))
      .WillOnce(testing::Return(fc::outcome::success(res)));
  EXPECT_CALL(*state_tree_, get(Eq(block_miner_)))
      .WillOnce(testing::Return(fc::outcome::success(miner_actor)));
  EXPECT_CALL(*state_tree_, set(_, _))
      .Times(3)
      .WillRepeatedly(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_TRUE_1(runtime_->send(to_address, method, params, amount));
}

/**
 * @given Runtime with actors and sender balance is zero
 * @when send() is called with transfer
 * @then Error NOT_ENOUGH_FUNDS returned
 */
TEST_F(RuntimeTest, sendNotEnoughFunds) {
  Address to_address{fc::primitives::address::TESTNET, 345};
  MethodNumber method{123};
  MethodParams params{};
  BigInt amount{100500};

  Actor from_actor{fc::vm::actor::kInitCodeCid, ActorSubstateCID{}, 0, 0};
  Actor to_actor{fc::vm::actor::kInitCodeCid, ActorSubstateCID{}, 0, 0};

  EXPECT_CALL(*state_tree_, flush())
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(*state_tree_, get(Eq(message_.to)))
      .WillOnce(testing::Return(fc::outcome::success(from_actor)));
  EXPECT_CALL(*state_tree_, get(Eq(to_address)))
      .WillOnce(testing::Return(fc::outcome::success(to_actor)));

  EXPECT_OUTCOME_ERROR(RuntimeError::NOT_ENOUGH_FUNDS,
                       runtime_->send(to_address, method, params, amount));
}

/**
 * @given Runtime with initial state
 * @when Commit new state
 * @then State is updated
 */
TEST_F(RuntimeTest, Commit) {
  auto new_state = ActorSubstateCID{"010001020002"_cid};
  EXPECT_NE(runtime_->getCurrentActorState(), new_state);
  EXPECT_OUTCOME_TRUE_1(runtime_->commit(new_state));
  EXPECT_EQ(runtime_->getCurrentActorState(), new_state);
}
