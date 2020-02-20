/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"

#include <gtest/gtest.h>
#include "codec/cbor/cbor.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"

using fc::CID;
using fc::adt::TokenAmount;
using fc::common::Buffer;
using fc::primitives::address::Address;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsDatastore;
using fc::vm::VMExitCode;
using fc::vm::actor::Actor;
using fc::vm::actor::ActorSubstateCID;
using fc::vm::actor::encodeActorParams;
using fc::vm::actor::kAccountCodeCid;
using fc::vm::actor::kCronAddress;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::kStorageMinerCodeCid;
using fc::vm::actor::builtin::miner::GetControlAddressesReturn;
using fc::vm::actor::builtin::miner::kGetControlAddresses;
using fc::vm::actor::builtin::storage_power::AddBalanceParameters;
using fc::vm::actor::builtin::storage_power::ConstructParameters;
using fc::vm::actor::builtin::storage_power::StoragePowerActor;
using fc::vm::actor::builtin::storage_power::StoragePowerActorMethods;
using fc::vm::actor::builtin::storage_power::StoragePowerActorState;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::InvocationOutput;
using fc::vm::runtime::MethodParams;
using fc::vm::runtime::MockRuntime;
using ::testing::_;
using ::testing::Eq;

class StoragePowerActorTest : public ::testing::Test {
 public:
  // Captures CID in mock call
  fc::outcome::result<void> captureCid(const ActorSubstateCID &cid) {
    captured_cid = cid;
    return fc::outcome::success();
  }

  // Returnes captured CID value
  ActorSubstateCID getCapturedCid() {
    return captured_cid;
  }

  /**
   * Creates actor state and adds one miner
   * @return Miner address
   */
  Address createStateWithMiner() {
    Address miner_address = Address::makeBls(
        "2222222222222222222222222222222222222222"
        "2222222222222222222222222222222222222222"
        "2222222222222222"_blob48);
    EXPECT_OUTCOME_TRUE(actor_empty_state,
                        StoragePowerActor::createEmptyState(datastore));
    StoragePowerActor actor(datastore, actor_empty_state);
    EXPECT_OUTCOME_TRUE_1(actor.addMiner(miner_address));
    EXPECT_OUTCOME_TRUE(actor_state, actor.flushState());
    EXPECT_OUTCOME_TRUE(actor_head_cid, datastore->setCbor(actor_state));
    caller.head = ActorSubstateCID{actor_head_cid};
    return miner_address;
  }

  Actor caller;
  Address caller_address = Address::makeBls(
      "1234567890123456789012345678901234567890"
      "1234567890123456789012345678901234567890"
      "1122334455667788"_blob48);

  std::shared_ptr<IpfsDatastore> datastore =
      std::make_shared<InMemoryDatastore>();
  MockRuntime runtime;

 private:
  ActorSubstateCID captured_cid;
};

/**
 * @given runtime and StoragePowerActor
 * @when constructor is called with caller actor different from SystemActor
 * @then Error returned
 */
TEST_F(StoragePowerActorTest, ConstructorWrongCaller) {
  ConstructParameters params;
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  // return != kInitAddress
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(kCronAddress));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER,
      StoragePowerActorMethods::construct(caller, runtime, encoded_params));
}

/**
 * @given runtime and StoragePowerActor
 * @when constructor is called
 * @then empty state is created
 */
TEST_F(StoragePowerActorTest, Constructor) {
  ConstructParameters params;
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(kInitAddress));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  // commit and capture state CID
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(::testing::Invoke(this, &StoragePowerActorTest::captureCid));

  EXPECT_OUTCOME_TRUE_1(
      StoragePowerActorMethods::construct(caller, runtime, encoded_params));

  // inspect state
  ActorSubstateCID state_cid = getCapturedCid();
  EXPECT_OUTCOME_TRUE(state,
                      datastore->getCbor<StoragePowerActorState>(state_cid));
  StoragePowerActor actor(datastore, state);
  EXPECT_OUTCOME_TRUE(cron_events, actor.getCronEvents());
  EXPECT_TRUE(cron_events.empty());
  EXPECT_OUTCOME_TRUE(fault_miners, actor.getFaultMiners());
  EXPECT_TRUE(fault_miners.empty());
  EXPECT_OUTCOME_TRUE(claims, actor.getClaims());
  EXPECT_TRUE(claims.empty());
  EXPECT_OUTCOME_TRUE(miners, actor.getMiners());
  EXPECT_TRUE(miners.empty());
}

/**
 * @given runtime and StoragePowerActor
 * @when addBalance is called with miner different from StorageMinerCodeId
 * @then Error returned
 */
TEST_F(StoragePowerActorTest, AddBalanceWrongParams) {
  AddBalanceParameters params;
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  // not a miner CodeCid
  EXPECT_CALL(runtime, getActorCodeID(_))
      .WillOnce(testing::Return(fc::outcome::success(kAccountCodeCid)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT,
      StoragePowerActorMethods::addBalance(caller, runtime, encoded_params));
}

/**
 * @given runtime and StoragePowerActor
 * @when Internal error is raised
 * @then Internal error returned
 */
TEST_F(StoragePowerActorTest, AddBalanceInternalError) {
  Address miner_address = Address::makeBls(
      "2222222222222222222222222222222222222222"
      "2222222222222222222222222222222222222222"
      "2222222222222222"_blob48);

  AddBalanceParameters params{miner_address};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getActorCodeID(_))
      .WillOnce(testing::Return(fc::outcome::success(kStorageMinerCodeCid)));
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(caller_address));
  EXPECT_CALL(runtime,
              send(Eq(miner_address),
                   Eq(kGetControlAddresses),
                   Eq(MethodParams{}),
                   Eq(TokenAmount{0})))
      .WillOnce(testing::Return(fc::outcome::failure(VMExitCode::_)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::_,
      StoragePowerActorMethods::addBalance(caller, runtime, encoded_params));
}

/**
 * @given runtime and StoragePowerActor state with miner
 * @when addBalance is called
 * @then balance is added
 */
TEST_F(StoragePowerActorTest, AddBalanceWrong) {
  Address miner_address = createStateWithMiner();
  TokenAmount amount_to_add{1334};

  AddBalanceParameters params{miner_address};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getActorCodeID(Eq(miner_address)))
      .WillOnce(testing::Return(fc::outcome::success(kStorageMinerCodeCid)));
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(caller_address));
  // shared::requestMinerControlAddress
  GetControlAddressesReturn get_controll_address_return{caller_address,
                                                        miner_address};
  EXPECT_OUTCOME_TRUE(encoded_get_controll_address_return,
                      fc::codec::cbor::encode(get_controll_address_return));
  EXPECT_CALL(runtime,
              send(Eq(miner_address),
                   Eq(kGetControlAddresses),
                   Eq(MethodParams{}),
                   Eq(TokenAmount{0})))
      .WillOnce(testing::Return(fc::outcome::success(
          InvocationOutput{Buffer{encoded_get_controll_address_return}})));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  // get message
  UnsignedMessage message{miner_address, caller_address, 0, amount_to_add};
  EXPECT_CALL(runtime, getMessage()).WillOnce(testing::Return(message));

  // commit and capture state CID
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(::testing::Invoke(this, &StoragePowerActorTest::captureCid));

  EXPECT_OUTCOME_TRUE_1(
      StoragePowerActorMethods::addBalance(caller, runtime, encoded_params));

  // inspect state
  ActorSubstateCID state_cid = getCapturedCid();
  EXPECT_OUTCOME_TRUE(state,
                      datastore->getCbor<StoragePowerActorState>(state_cid));
  StoragePowerActor actor(datastore, state);
  EXPECT_OUTCOME_EQ(actor.getMinerBalance(miner_address), amount_to_add);
}
