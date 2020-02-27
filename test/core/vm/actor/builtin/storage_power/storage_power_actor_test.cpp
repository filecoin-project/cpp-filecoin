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
#include "vm/actor/builtin/init/init_actor.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/actor/builtin/storage_power/policy.hpp"

using fc::CID;
using fc::common::Buffer;
using fc::power::Power;
using fc::primitives::DealWeight;
using fc::primitives::EpochDuration;
using fc::primitives::SectorStorageWeightDesc;
using fc::primitives::TokenAmount;
using fc::primitives::address::Address;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsDatastore;
using fc::vm::VMExitCode;
using fc::vm::actor::Actor;
using fc::vm::actor::ActorSubstateCID;
using fc::vm::actor::encodeActorParams;
using fc::vm::actor::encodeActorReturn;
using fc::vm::actor::kAccountCodeCid;
using fc::vm::actor::kBurntFundsActorAddress;
using fc::vm::actor::kCronAddress;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::kSendMethodNumber;
using fc::vm::actor::kStorageMinerCodeCid;
using fc::vm::actor::builtin::miner::GetControlAddressesReturn;
using fc::vm::actor::builtin::miner::kGetControlAddressesMethodNumber;
using fc::vm::actor::builtin::miner::PeerId;
using fc::vm::actor::builtin::storage_power::AddBalanceParameters;
using fc::vm::actor::builtin::storage_power::Claim;
using fc::vm::actor::builtin::storage_power::CreateMinerParameters;
using fc::vm::actor::builtin::storage_power::CreateMinerReturn;
using fc::vm::actor::builtin::storage_power::DeleteMinerParameters;
using fc::vm::actor::builtin::storage_power::kEpochTotalExpectedReward;
using fc::vm::actor::builtin::storage_power::kPledgeFactor;
using fc::vm::actor::builtin::storage_power::OnSectorProveCommitParameters;
using fc::vm::actor::builtin::storage_power::OnSectorProveCommitReturn;
using fc::vm::actor::builtin::storage_power::OnSectorTerminateParameters;
using fc::vm::actor::builtin::storage_power::SectorTerminationType;
using fc::vm::actor::builtin::storage_power::StoragePowerActor;
using fc::vm::actor::builtin::storage_power::StoragePowerActorMethods;
using fc::vm::actor::builtin::storage_power::StoragePowerActorState;
using fc::vm::actor::builtin::storage_power::WithdrawBalanceParameters;
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
   * Create sempty actor state
   */
  void createEmptyState() {
    EXPECT_OUTCOME_TRUE(empty_state,
                        StoragePowerActor::createEmptyState(datastore))
    EXPECT_OUTCOME_TRUE(state_cid, datastore->setCbor(empty_state));
    actor_head_cid = ActorSubstateCID{state_cid};
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
    StoragePowerActor power_actor(datastore, actor_empty_state);
    EXPECT_OUTCOME_TRUE_1(power_actor.addMiner(miner_address));
    EXPECT_OUTCOME_TRUE(actor_state, power_actor.flushState());
    EXPECT_OUTCOME_TRUE(new_actor_head_cid, datastore->setCbor(actor_state));
    actor_head_cid = ActorSubstateCID{new_actor_head_cid};
    return miner_address;
  }

  /**
   * Set total network power for power actor state
   * @param power to set
   */
  void setNetworkPower(const Power &power) {
    EXPECT_OUTCOME_TRUE(
        actor_state,
        datastore->getCbor<StoragePowerActorState>(actor_head_cid));
    actor_state.total_network_power = power;
    StoragePowerActor power_actor(datastore, actor_state);
    EXPECT_OUTCOME_TRUE(actor_new_state, power_actor.flushState());
    EXPECT_OUTCOME_TRUE(new_actor_head_cid, datastore->setCbor(actor_new_state));
    actor_head_cid = ActorSubstateCID{new_actor_head_cid};
  }

  /**
   * Add balance to miner and save state
   * @param miner address
   * @param amount to add
   */
  void addBalance(const Address &miner, const TokenAmount &amount) {
    EXPECT_OUTCOME_TRUE(
        actor_state,
        datastore->getCbor<StoragePowerActorState>(actor_head_cid));
    StoragePowerActor power_actor(datastore, actor_state);
    EXPECT_OUTCOME_TRUE_1(power_actor.addMinerBalance(miner, amount));
    EXPECT_OUTCOME_TRUE(actor_new_state, power_actor.flushState());
    EXPECT_OUTCOME_TRUE(new_actor_head_cid, datastore->setCbor(actor_new_state));
    actor_head_cid = ActorSubstateCID{new_actor_head_cid};
  }

  /**
   * Get miner balance with given state
   * @param state_root CID to StoragePowerActor state
   * @param miner_address address
   * @return balance
   */
  TokenAmount getBalance(const CID &state_root, const Address &miner_address) {
    EXPECT_OUTCOME_TRUE(state,
                        datastore->getCbor<StoragePowerActorState>(state_root));
    StoragePowerActor power_actor(datastore, state);
    EXPECT_OUTCOME_TRUE(balance, power_actor.getMinerBalance(miner_address));
    return balance;
  }

  /**
   * Set claim to miner and save new state
   * @param miner address
   * @param claim to set
   */
  void setClaim(const Address &miner, const Claim &claim) {
    EXPECT_OUTCOME_TRUE(
        actor_state,
        datastore->getCbor<StoragePowerActorState>(actor_head_cid));
    StoragePowerActor power_actor(datastore, actor_state);
    EXPECT_OUTCOME_TRUE_1(power_actor.setClaim(miner, claim));
    EXPECT_OUTCOME_TRUE(actor_new_state, power_actor.flushState());
    EXPECT_OUTCOME_TRUE(new_actor_head_cid, datastore->setCbor(actor_new_state));
    actor_head_cid = ActorSubstateCID{new_actor_head_cid};
  }

  /** Storage poer actor state CID */
  ActorSubstateCID actor_head_cid;

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
  // return != kInitAddress
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(kCronAddress));

  EXPECT_OUTCOME_ERROR(VMExitCode::STORAGE_POWER_ACTOR_WRONG_CALLER,
                       StoragePowerActorMethods::construct(runtime, {}));
}

/**
 * @given runtime and StoragePowerActor
 * @when constructor is called
 * @then empty state is created
 */
TEST_F(StoragePowerActorTest, Constructor) {
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(kInitAddress));
  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  // commit and capture state CID
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(::testing::Invoke(this, &StoragePowerActorTest::captureCid));

  EXPECT_OUTCOME_TRUE_1(StoragePowerActorMethods::construct(runtime, {}));

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
      StoragePowerActorMethods::addBalance(runtime, encoded_params));
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
                   Eq(kGetControlAddressesMethodNumber),
                   Eq(MethodParams{}),
                   Eq(TokenAmount{0})))
      .WillOnce(testing::Return(fc::outcome::failure(VMExitCode::_)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::_,
      StoragePowerActorMethods::addBalance(runtime, encoded_params));
}

/**
 * @given runtime and StoragePowerActor state with miner
 * @when addBalance is called
 * @then balance is added
 */
TEST_F(StoragePowerActorTest, AddBalanceSuccess) {
  Address miner_address = createStateWithMiner();
  TokenAmount amount_to_add{1334};

  AddBalanceParameters params{miner_address};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getCurrentActorState())
      .WillOnce(::testing::Return(actor_head_cid));
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
                   Eq(kGetControlAddressesMethodNumber),
                   Eq(MethodParams{}),
                   Eq(TokenAmount{0})))
      .WillOnce(testing::Return(fc::outcome::success(
          InvocationOutput{Buffer{encoded_get_controll_address_return}})));
  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(3)
      .WillRepeatedly(testing::Return(datastore));
  // get message
  UnsignedMessage message{miner_address, caller_address, 0, amount_to_add};
  EXPECT_CALL(runtime, getMessage()).WillOnce(testing::Return(message));

  // commit and capture state CID
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(::testing::Invoke(this, &StoragePowerActorTest::captureCid));

  EXPECT_OUTCOME_TRUE_1(
      StoragePowerActorMethods::addBalance(runtime, encoded_params));

  // inspect state
  ActorSubstateCID state_cid = getCapturedCid();
  EXPECT_EQ(getBalance(state_cid, miner_address), amount_to_add);
}

/**
 * @given runtime and StoragePowerActor state with miner
 * @when withdrawBalance is called with negative requested amount
 * @then error ILLEGAL_ARGUMENT returned
 */
TEST_F(StoragePowerActorTest, WithdrawBalanceNegative) {
  Address miner_address = createStateWithMiner();
  TokenAmount amount_to_withdraw{-1334};

  WithdrawBalanceParameters params{miner_address, amount_to_withdraw};
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
                   Eq(kGetControlAddressesMethodNumber),
                   Eq(MethodParams{}),
                   Eq(TokenAmount{0})))
      .WillOnce(testing::Return(fc::outcome::success(
          InvocationOutput{Buffer{encoded_get_controll_address_return}})));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT,
      StoragePowerActorMethods::withdrawBalance(runtime, encoded_params));
}

/**
 * @given runtime and StoragePowerActor state with miner with some balance
 * @when withdrawBalance is called
 * @then balance withdrawed
 */
TEST_F(StoragePowerActorTest, WithdrawBalanceSuccess) {
  Address miner_address = createStateWithMiner();
  TokenAmount amount{1334};
  addBalance(miner_address, amount);

  WithdrawBalanceParameters params{miner_address, amount};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getActorCodeID(Eq(miner_address)))
      .WillOnce(testing::Return(fc::outcome::success(kStorageMinerCodeCid)));
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(caller_address));
  EXPECT_CALL(runtime, getCurrentActorState())
      .WillOnce(::testing::Return(actor_head_cid));
  // shared::requestMinerControlAddress
  GetControlAddressesReturn get_controll_address_return{caller_address,
                                                        miner_address};
  EXPECT_OUTCOME_TRUE(encoded_get_controll_address_return,
                      fc::codec::cbor::encode(get_controll_address_return));
  EXPECT_CALL(runtime,
              send(Eq(miner_address),
                   Eq(kGetControlAddressesMethodNumber),
                   Eq(MethodParams{}),
                   Eq(TokenAmount{0})))
      .WillOnce(testing::Return(fc::outcome::success(
          InvocationOutput{Buffer{encoded_get_controll_address_return}})));
  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(3)
      .WillRepeatedly(testing::Return(datastore));
  // transfer amount
  EXPECT_CALL(runtime,
              send(Eq(caller_address),
                   Eq(kSendMethodNumber),
                   Eq(MethodParams{}),
                   Eq(amount)))
      .WillOnce(testing::Return(fc::outcome::success(InvocationOutput{})));
  // commit and capture state CID
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(::testing::Invoke(this, &StoragePowerActorTest::captureCid));

  EXPECT_OUTCOME_TRUE_1(
      StoragePowerActorMethods::withdrawBalance(runtime, encoded_params));

  // inspect state
  ActorSubstateCID state_cid = getCapturedCid();
  EXPECT_EQ(getBalance(state_cid, miner_address), TokenAmount{0});
}

/**
 * @given runtime and StoragePowerActor
 * @when createMiner is called
 * @then miner is created
 */
TEST_F(StoragePowerActorTest, CreateMinerSuccess) {
  createEmptyState();

  Address worker_address{Address::makeFromId(1334)};
  uint64_t sector_size = 2446;
  PeerId peer_id = "peer_id";
  CreateMinerParameters create_miner_params{
      worker_address, sector_size, peer_id};
  EXPECT_OUTCOME_TRUE(encoded_create_miner_params,
                      encodeActorParams(create_miner_params));

  Address any_address_1 = Address::makeBls(
      "1111111111111111111111111111111111111111"
      "1111111111111111111111111111111111111111"
      "1111111111111111"_blob48);
  Address any_address_2 = Address::makeBls(
      "2222222222222222222222222222222222222222"
      "2222222222222222222222222222222222222222"
      "2222222222222222"_blob48);
  TokenAmount amount{100200};
  UnsignedMessage message{any_address_1, caller_address, 0, amount};
  EXPECT_CALL(runtime, getMessage()).WillOnce(testing::Return(message));

  // return immediate caller is signable code id
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(caller_address));
  EXPECT_CALL(runtime, getActorCodeID(Eq(caller_address)))
      .WillOnce(testing::Return(kAccountCodeCid));

  EXPECT_CALL(runtime, getCurrentActorState())
      .WillOnce(::testing::Return(actor_head_cid));

  // check send params
  fc::vm::actor::builtin::miner::ConstructorParams construct_params{
      caller_address, worker_address, sector_size, peer_id};
  EXPECT_OUTCOME_TRUE(encoded_construct_params,
                      encodeActorParams(construct_params));

  runtime.expectSendM<fc::vm::actor::builtin::init::Exec>(
      kInitAddress,
      {kStorageMinerCodeCid, encoded_construct_params},
      0,
      {any_address_1, any_address_2});

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(3)
      .WillRepeatedly(testing::Return(datastore));

  // commit and capture state CID
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(::testing::Invoke(this, &StoragePowerActorTest::captureCid));

  // expected output
  CreateMinerReturn result{any_address_1, any_address_2};
  EXPECT_OUTCOME_TRUE(encoded_result, encodeActorReturn(result));

  EXPECT_OUTCOME_EQ(StoragePowerActorMethods::createMiner(
                        runtime, encoded_create_miner_params),
                    encoded_result);

  // inspect state
  ActorSubstateCID state_cid = getCapturedCid();
  EXPECT_EQ(getBalance(state_cid, any_address_1), amount);
}

/**
 * @given State and miner with claim power != 0
 * @when deleteMiner is called
 * @then Error FORBIDDEN returned
 */
TEST_F(StoragePowerActorTest, DeleteMinerClaimPowerNotZero) {
  Address miner_address = createStateWithMiner();
  Claim claim{100, 200};
  setClaim(miner_address, claim);

  DeleteMinerParameters delete_params{miner_address};
  EXPECT_OUTCOME_TRUE(encoded_delete_params, encodeActorParams(delete_params));

  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(caller_address));

  EXPECT_CALL(runtime, getCurrentActorState())
      .WillOnce(::testing::Return(actor_head_cid));

  // shared::requestMinerControlAddress
  GetControlAddressesReturn get_controll_address_return{caller_address,
                                                        miner_address};
  EXPECT_OUTCOME_TRUE(encoded_get_controll_address_return,
                      fc::codec::cbor::encode(get_controll_address_return));
  EXPECT_CALL(runtime,
              send(Eq(miner_address),
                   Eq(kGetControlAddressesMethodNumber),
                   Eq(MethodParams{}),
                   Eq(TokenAmount{0})))
      .WillOnce(testing::Return(fc::outcome::success(
          InvocationOutput{Buffer{encoded_get_controll_address_return}})));

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::STORAGE_POWER_FORBIDDEN,
      StoragePowerActorMethods::deleteMiner(runtime, encoded_delete_params));
}

/**
 * @given State and miner absent
 * @when deleteMiner is called
 * @then Error ILLEGAL_ARGUMENT
 */
TEST_F(StoragePowerActorTest, DeleteMinerNoMiner) {
  createEmptyState();
  Address miner_address = Address::makeBls(
      "1111111111111111111111111111111111111111"
      "1111111111111111111111111111111111111111"
      "1111111111111111"_blob48);

  DeleteMinerParameters delete_params{miner_address};
  EXPECT_OUTCOME_TRUE(encoded_delete_params, encodeActorParams(delete_params));

  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(caller_address));

  EXPECT_CALL(runtime, getCurrentActorState())
      .WillOnce(::testing::Return(actor_head_cid));

  // shared::requestMinerControlAddress
  GetControlAddressesReturn get_controll_address_return{caller_address,
                                                        miner_address};
  EXPECT_OUTCOME_TRUE(encoded_get_controll_address_return,
                      fc::codec::cbor::encode(get_controll_address_return));
  EXPECT_CALL(runtime,
              send(Eq(miner_address),
                   Eq(kGetControlAddressesMethodNumber),
                   Eq(MethodParams{}),
                   Eq(TokenAmount{0})))
      .WillOnce(testing::Return(fc::outcome::success(
          InvocationOutput{Buffer{encoded_get_controll_address_return}})));

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::STORAGE_POWER_ILLEGAL_ARGUMENT,
      StoragePowerActorMethods::deleteMiner(runtime, encoded_delete_params));
}

/**
 * @given State and miner
 * @when deleteMiner is called
 * @then miner deleted and miner balance is slashed
 */
TEST_F(StoragePowerActorTest, DeleteMinerSuccess) {
  Address miner_address = createStateWithMiner();
  TokenAmount balance = 100;
  addBalance(miner_address, balance);

  DeleteMinerParameters delete_params{miner_address};
  EXPECT_OUTCOME_TRUE(encoded_delete_params, encodeActorParams(delete_params));

  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(caller_address));

  EXPECT_CALL(runtime, getCurrentActorState())
      .WillOnce(::testing::Return(actor_head_cid));

  // shared::requestMinerControlAddress
  GetControlAddressesReturn get_controll_address_return{caller_address,
                                                        miner_address};
  EXPECT_OUTCOME_TRUE(encoded_get_controll_address_return,
                      fc::codec::cbor::encode(get_controll_address_return));
  EXPECT_CALL(runtime,
              send(Eq(miner_address),
                   Eq(kGetControlAddressesMethodNumber),
                   Eq(MethodParams{}),
                   Eq(TokenAmount{0})))
      .WillOnce(testing::Return(fc::outcome::success(
          InvocationOutput{Buffer{encoded_get_controll_address_return}})));
  EXPECT_CALL(
      runtime,
      send(Eq(miner_address),
           Eq(fc::vm::actor::builtin::miner::kOnDeleteMinerMethodNumber),
           Eq(MethodParams{}),
           Eq(TokenAmount{0})))
      .WillOnce(testing::Return(fc::outcome::success(InvocationOutput{})));
  EXPECT_CALL(runtime,
              send(Eq(kBurntFundsActorAddress),
                   Eq(kSendMethodNumber),
                   Eq(MethodParams{}),
                   Eq(TokenAmount{balance})))
      .WillOnce(testing::Return(fc::outcome::success(InvocationOutput{})));

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(3)
      .WillRepeatedly(testing::Return(datastore));

  // commit and capture state CID
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(::testing::Invoke(this, &StoragePowerActorTest::captureCid));

  EXPECT_OUTCOME_TRUE_1(
      StoragePowerActorMethods::deleteMiner(runtime, encoded_delete_params));

  // inspect state
  ActorSubstateCID state_cid = getCapturedCid();
  EXPECT_OUTCOME_TRUE(state,
                      datastore->getCbor<StoragePowerActorState>(state_cid));
  StoragePowerActor actor(datastore, state);
  EXPECT_OUTCOME_EQ(actor.hasMiner(miner_address), false);
}

/**
 * @given Runtime and state with miner
 * @when onSectorProofCommitSuccess called
 * @then miner claim has changed
 */
TEST_F(StoragePowerActorTest, OnSectorProofCommitSuccess) {
  Address miner_address = createStateWithMiner();
  Power network_power{10};
  setNetworkPower(network_power);
  uint64_t sector_size{2};
  EpochDuration duration{3};
  SectorStorageWeightDesc weight_descr{sector_size, duration, 0};
  OnSectorProveCommitParameters parameters{weight_descr};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(parameters));

  // return immediate caller is signable code id
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(caller_address));
  EXPECT_CALL(runtime, getActorCodeID(Eq(caller_address)))
      .WillOnce(testing::Return(kStorageMinerCodeCid));

  EXPECT_CALL(runtime, getCurrentActorState())
      .WillOnce(::testing::Return(actor_head_cid));

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(3)
      .WillRepeatedly(testing::Return(datastore));

  UnsignedMessage message{caller_address, miner_address, 0, 0};
  EXPECT_CALL(runtime, getMessage()).WillOnce(testing::Return(message));

  // commit and capture state CID
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(::testing::Invoke(this, &StoragePowerActorTest::captureCid));

  // expected output
  TokenAmount pledge = sector_size * duration * kEpochTotalExpectedReward
                       * kPledgeFactor / network_power;
  OnSectorProveCommitReturn result{pledge};
  EXPECT_OUTCOME_TRUE(encoded_result, encodeActorReturn(result));

  EXPECT_OUTCOME_EQ(
      StoragePowerActorMethods::onSectorProveCommit(runtime, encoded_params),
      encoded_result);

  // inspect state
  ActorSubstateCID state_cid = getCapturedCid();
  EXPECT_OUTCOME_TRUE(state,
                      datastore->getCbor<StoragePowerActorState>(state_cid));
  StoragePowerActor actor(datastore, state);
  EXPECT_OUTCOME_TRUE(claim, actor.getClaim(miner_address));
  EXPECT_EQ(claim.pledge, pledge);
  EXPECT_EQ(claim.power, sector_size);
}

/**
 * @givenRuntime and state with miner
 * @when OnSectorTerminateSuccess called
 * @then miner balance slashed
 */
TEST_F(StoragePowerActorTest, OnSectorTerminateSuccess) {
  Address miner_address = createStateWithMiner();
  Power initial_power{100};
  TokenAmount initial_pledge{100};
  setClaim(miner_address, Claim{initial_power, initial_pledge});

  SectorStorageWeightDesc weight_descr_1{1, 1, 1};
  SectorStorageWeightDesc weight_descr_2{2, 2, 2};
  std::vector<SectorStorageWeightDesc> weights{weight_descr_1, weight_descr_2};
  TokenAmount pledge{10};
  OnSectorTerminateParameters parameters{
      SectorTerminationType::SECTOR_TERMINATION_EXPIRED, weights, pledge};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(parameters));

  // return immediate caller is signable code id
  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(caller_address));
  EXPECT_CALL(runtime, getActorCodeID(Eq(caller_address)))
      .WillOnce(testing::Return(kStorageMinerCodeCid));

  EXPECT_CALL(runtime, getCurrentActorState())
      .WillOnce(::testing::Return(actor_head_cid));

  UnsignedMessage message{caller_address, miner_address, 0, 0};
  EXPECT_CALL(runtime, getMessage()).WillOnce(testing::Return(message));

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(3)
      .WillRepeatedly(testing::Return(datastore));

  // commit and capture state CID
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(::testing::Invoke(this, &StoragePowerActorTest::captureCid));

  EXPECT_OUTCOME_TRUE_1(
      StoragePowerActorMethods::onSectorTerminate(runtime, encoded_params));

  // inspect state
  ActorSubstateCID state_cid = getCapturedCid();
  EXPECT_OUTCOME_TRUE(state,
                      datastore->getCbor<StoragePowerActorState>(state_cid));
  StoragePowerActor actor(datastore, state);

  EXPECT_OUTCOME_TRUE(claim, actor.getClaim(miner_address));
  EXPECT_EQ(claim.pledge, initial_pledge - pledge);
  EXPECT_EQ(
      claim.power,
      initial_power - weight_descr_1.sector_size - weight_descr_2.sector_size);
}
