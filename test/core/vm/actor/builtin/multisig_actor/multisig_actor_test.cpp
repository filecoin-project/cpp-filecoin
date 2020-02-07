/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/multisig/multisig_actor.hpp"

#include <gtest/gtest.h>
#include "primitives/address/address.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/storage/ipfs/ipfs_datastore_mock.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor_method.hpp"

using fc::CID;
using fc::common::Buffer;
using fc::primitives::BigInt;
using fc::primitives::ChainEpoch;
using fc::primitives::EpochDuration;
using fc::primitives::address::Address;
using fc::storage::ipfs::MockIpfsDatastore;
using fc::vm::VMExitCode;
using fc::vm::actor::Actor;
using fc::vm::actor::encodeActorParams;
using fc::vm::actor::kAccountCodeCid;
using fc::vm::actor::kCronAddress;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::MethodNumber;
using fc::vm::actor::MethodParams;
using fc::vm::actor::builtin::multisig::AddSignerParameters;
using fc::vm::actor::builtin::multisig::ChangeThresholdParameters;
using fc::vm::actor::builtin::multisig::ConstructParameteres;
using fc::vm::actor::builtin::multisig::MultiSigActor;
using fc::vm::actor::builtin::multisig::MultiSignatureActorState;
using fc::vm::actor::builtin::multisig::MultiSignatureTransaction;
using fc::vm::actor::builtin::multisig::ProposeParameters;
using fc::vm::actor::builtin::multisig::RemoveSignerParameters;
using fc::vm::actor::builtin::multisig::SwapSignerParameters;
using fc::vm::actor::builtin::multisig::TransactionNumber;
using fc::vm::actor::builtin::multisig::TransactionNumberParameters;
using fc::vm::runtime::InvocationOutput;
using fc::vm::runtime::MockRuntime;
using ::testing::_;
using ::testing::Eq;

/**
 * Match encoded MultiSignatureActorState is expected one
 * Used for IpldStore set state.
 */
MATCHER_P(MultiSignatureActorStateMatcher,
          expected,
          "Match MultisignatureActorState") {
  MultiSignatureActorState actual =
      fc::codec::cbor::decode<MultiSignatureActorState>(arg).value();
  return actual.signers == expected.signers
         && actual.threshold == expected.threshold
         && actual.next_transaction_id == expected.next_transaction_id
         && actual.initial_balance == expected.initial_balance
         && actual.start_epoch == expected.start_epoch
         && actual.unlock_duration == expected.unlock_duration
         && actual.pending_transactions == expected.pending_transactions;
}

class MultisigActorTest : public ::testing::Test {
 protected:
  Actor actor;
  Address address = Address::makeBls(
      "1234567890123456789012345678901234567890"
      "1234567890123456789012345678901234567890"
      "1122334455667788"_blob48);
  Address to_address{kCronAddress};  // arbitrary address
  MockRuntime runtime;
  std::shared_ptr<MockIpfsDatastore> datastore =
      std::make_shared<MockIpfsDatastore>();
  MethodNumber method_number{1};
  MethodParams method_params{Buffer{"0102"_unhex}};
  size_t default_threshold{1};
  TransactionNumber default_next_transaction_id{1};
  BigInt default_initial_balance{0};
  ChainEpoch default_start_epoch{0};
  EpochDuration default_unlock_duration{0};
  std::vector<MultiSignatureTransaction> default_pending_transactions{};
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
      VMExitCode::MULTISIG_ACTOR_WRONG_CALLER,
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

  EXPECT_OUTCOME_ERROR(VMExitCode::DECODE_ACTOR_PARAMS_ERROR,
                       MultiSigActor::construct(actor, runtime, wrong_params));
}

/**
 * @given Runtime and multisig actor
 * @when constructor is called with threshold more than signers
 * @then error returned
 */
TEST_F(MultisigActorTest, ConstructWrongThreshold) {
  std::vector<Address> signers{address};
  size_t threshold{5};
  ConstructParameteres params{signers, threshold, default_unlock_duration};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getImmediateCaller())
      .WillOnce(testing::Return(kInitAddress));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT,
      MultiSigActor::construct(actor, runtime, encoded_params));
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

/**
 * @given Runtime and multisig actor
 * @when propose is called with immediate caller is not signable
 * @then error WRONG_CALLER returned
 */
TEST_F(MultisigActorTest, ProposeWrongCaller) {
  ProposeParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_WRONG_CALLER,
                       MultiSigActor::propose(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when propose is called with wrong encoded constructor params
 * @then serialization error returned
 */
TEST_F(MultisigActorTest, ProposetWrongParams) {
  MethodParams wrong_params{"DEAD"_unhex};
  actor.code = kAccountCodeCid;

  EXPECT_OUTCOME_ERROR(VMExitCode::DECODE_ACTOR_PARAMS_ERROR,
                       MultiSigActor::propose(actor, runtime, wrong_params));
}

/**
 * @given Runtime and multisig actor
 * @when propose is called with not signer
 * @then error returned
 */
TEST_F(MultisigActorTest, ProposetWrongSigner) {
  ProposeParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  actor.code = kAccountCodeCid;
  MultiSignatureActorState actor_state{};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_FORBIDDEN,
                       MultiSigActor::propose(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor and no funds and no lock period
 * @when propose is called with threshold 1 and value transferred
 * @then error insufficient funds
 */
TEST_F(MultisigActorTest, ProposeSendInsufficientFunds) {
  BigInt actor_balance{1};
  BigInt value_to_send{100500};  // > actor state balance
  actor.balance = actor_balance;
  ProposeParameters params{
      to_address, value_to_send, method_number, method_params};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  actor.code = kAccountCodeCid;
  TransactionNumber tx_number{13};
  MultiSignatureActorState actor_state{std::vector{address},
                                       1,
                                       tx_number,
                                       actor_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_INSUFFICIENT_FUNDS,
                       MultiSigActor::propose(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor and funds locked
 * @when propose is called with threshold 1 and value transferred
 * @then error funds locked
 */
TEST_F(MultisigActorTest, ProposeSendFundsLocked) {
  BigInt actor_balance{200};
  BigInt value_to_send{200};
  actor.balance = actor_balance;
  ProposeParameters params{
      to_address, value_to_send, method_number, method_params};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  actor.code = kAccountCodeCid;
  TransactionNumber tx_number{13};
  ChainEpoch start_epoch{42};
  ChainEpoch epoch{43};  // < start_epoch + unlock_duration
  EpochDuration unlock_duration{10};
  MultiSignatureActorState actor_state{std::vector{address},
                                       1,
                                       tx_number,
                                       actor_balance,
                                       start_epoch,
                                       unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentEpoch()).WillOnce(testing::Return(epoch));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_INSUFFICIENT_FUNDS,
                       MultiSigActor::propose(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor and funds locked because start epoch more
 * than current epoch
 * @when propose is called with threshold 1 and value transferred
 * @then error funds locked
 */
TEST_F(MultisigActorTest, ProposeSendFundsLockedStartEpoch) {
  BigInt actor_balance{200};
  BigInt value_to_send{200};
  actor.balance = actor_balance;
  ProposeParameters params{
      to_address, value_to_send, method_number, method_params};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  actor.code = kAccountCodeCid;
  TransactionNumber tx_number{13};
  ChainEpoch start_epoch{42};
  ChainEpoch epoch{10};  // < start_epoch
  EpochDuration unlock_duration{10};
  MultiSignatureActorState actor_state{std::vector{address},
                                       1,
                                       tx_number,
                                       actor_balance,
                                       start_epoch,
                                       unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentEpoch()).WillOnce(testing::Return(epoch));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_INSUFFICIENT_FUNDS,
                       MultiSigActor::propose(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when propose is called with threshold 1
 * @then transaction is sent
 */
TEST_F(MultisigActorTest, ProposeSendFundsEnough) {
  BigInt actor_balance{100};
  BigInt value_to_send{50};
  actor.balance = actor_balance;
  ProposeParameters params{
      to_address, value_to_send, method_number, method_params};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  actor.code = kAccountCodeCid;
  TransactionNumber tx_number{13};
  MultiSignatureActorState actor_state{std::vector{address},
                                       1,
                                       tx_number,
                                       actor_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  MultiSignatureActorState expected_state{std::vector{address},
                                          1,
                                          tx_number + 1,
                                          actor_balance,
                                          default_start_epoch,
                                          default_unlock_duration,
                                          default_pending_transactions};

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(*datastore,
              set(_, MultiSignatureActorStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentEpoch())
      .WillOnce(testing::Return(ChainEpoch{42}));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime,
              send(Eq(to_address),
                   Eq(method_number),
                   Eq(method_params),
                   Eq(value_to_send)))
      .WillOnce(testing::Return(fc::outcome::success(InvocationOutput{})));

  EXPECT_OUTCOME_TRUE(expected, fc::codec::cbor::encode(tx_number));
  EXPECT_OUTCOME_EQ(MultiSigActor::propose(actor, runtime, encoded_params),
                    InvocationOutput{Buffer{expected}});
}

/**
 * @given Runtime and multisig actor
 * @when propose is called with threshold 2
 * @then transaction is pending
 */
TEST_F(MultisigActorTest, ProposePending) {
  BigInt actor_balance{100};
  BigInt value_to_send{50};
  actor.balance = actor_balance;
  ProposeParameters params{
      to_address, value_to_send, method_number, method_params};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  actor.code = kAccountCodeCid;
  TransactionNumber tx_number{13};
  MultiSignatureActorState actor_state{std::vector{address},
                                       2,
                                       tx_number,
                                       actor_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  MultiSignatureTransaction pending_tx{tx_number,
                                       to_address,
                                       value_to_send,
                                       method_number,
                                       method_params,
                                       std::vector{address}};
  MultiSignatureActorState expected_state{std::vector{address},
                                          2,
                                          tx_number + 1,
                                          actor_balance,
                                          0,
                                          0,
                                          std::vector{pending_tx}};

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(*datastore,
              set(_, MultiSignatureActorStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_TRUE(expected, fc::codec::cbor::encode(tx_number));
  EXPECT_OUTCOME_EQ(MultiSigActor::propose(actor, runtime, encoded_params),
                    InvocationOutput{Buffer{expected}});
}

/**
 * @given Runtime and multisig actor
 * @when approve is called with immediate caller is not signable
 * @then error WRONG_CALLER returned
 */
TEST_F(MultisigActorTest, ApproveWrongCaller) {
  TransactionNumberParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_WRONG_CALLER,
                       MultiSigActor::approve(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when approve is called with wrong signer
 * @then error returned
 */
TEST_F(MultisigActorTest, ApproveWrongSigner) {
  TransactionNumberParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  actor.code = kAccountCodeCid;
  MultiSignatureActorState actor_state{std::vector{kInitAddress},
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_FORBIDDEN,
                       MultiSigActor::approve(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when approve is called with wrong tx_number
 * @then error returned
 */
TEST_F(MultisigActorTest, ApproveWrongTxNumber) {
  TransactionNumberParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  actor.code = kAccountCodeCid;
  // no pending txs in state
  MultiSignatureActorState actor_state{std::vector{address},
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_NOT_FOUND,
                       MultiSigActor::approve(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor and pending tx
 * @when approve is called with caller already signed
 * @then error returned
 */
TEST_F(MultisigActorTest, ApproveAlreadySigned) {
  BigInt actor_balance{100};
  BigInt value_to_send{50};
  actor.balance = actor_balance;
  actor.code = kAccountCodeCid;
  std::vector<Address> signers{address, kInitAddress};
  size_t threshold{2};
  TransactionNumber pending_tx_number{13};
  TransactionNumber tx_number{13};
  // tx is approved by kInitAddress
  MultiSignatureTransaction pending_tx{pending_tx_number,
                                       to_address,
                                       value_to_send,
                                       method_number,
                                       method_params,
                                       std::vector{address}};
  MultiSignatureActorState actor_state{signers,
                                       threshold,
                                       tx_number,
                                       actor_balance,
                                       0,
                                       0,
                                       std::vector{pending_tx}};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));
  TransactionNumberParameters approve_params{pending_tx_number};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(approve_params));

  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_ILLEGAL_STATE,
                       MultiSigActor::approve(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor and pending tx
 * @when approve is called
 * @then transaction is signed, called and deleted from state
 */
TEST_F(MultisigActorTest, ApproveSunnyDay) {
  BigInt actor_balance{100};
  BigInt value_to_send{50};
  actor.balance = actor_balance;
  actor.code = kAccountCodeCid;
  std::vector<Address> signers{address, kInitAddress};
  size_t threshold{2};
  TransactionNumber pending_tx_number{13};
  TransactionNumber tx_number{13};
  // tx is approved by kInitAddress
  MultiSignatureTransaction pending_tx{pending_tx_number,
                                       to_address,
                                       value_to_send,
                                       method_number,
                                       method_params,
                                       std::vector{kInitAddress}};
  MultiSignatureActorState actor_state{signers,
                                       threshold,
                                       tx_number,
                                       actor_balance,
                                       0,
                                       0,
                                       std::vector{pending_tx}};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));
  TransactionNumberParameters approve_params{pending_tx_number};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(approve_params));

  // expected that pending tx is removed after sending
  MultiSignatureActorState expected_state{
      signers,
      threshold,
      tx_number,
      actor_balance,
      0,
      0,
      std::vector<MultiSignatureTransaction>{}};

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(*datastore,
              set(_, MultiSignatureActorStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentEpoch())
      .WillOnce(testing::Return(ChainEpoch{42}));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime,
              send(Eq(to_address),
                   Eq(method_number),
                   Eq(method_params),
                   Eq(value_to_send)))
      .WillOnce(testing::Return(fc::outcome::success(InvocationOutput{})));

  EXPECT_OUTCOME_EQ(MultiSigActor::approve(actor, runtime, encoded_params),
                    InvocationOutput{});
}

/**
 * @given Runtime and multisig actor
 * @when cancel is called with immediate caller is not signable
 * @then error WRONG_CALLER returned
 */
TEST_F(MultisigActorTest, CancelWrongCaller) {
  TransactionNumberParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_WRONG_CALLER,
                       MultiSigActor::cancel(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when cancel is called with wrong signer
 * @then error returned
 */
TEST_F(MultisigActorTest, CancelWrongSigner) {
  TransactionNumberParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  actor.code = kAccountCodeCid;
  MultiSignatureActorState actor_state{std::vector{kInitAddress},
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_FORBIDDEN,
                       MultiSigActor::cancel(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when cancel is called with wrong tx_number
 * @then error returned
 */
TEST_F(MultisigActorTest, CancelWrongTxNumber) {
  TransactionNumberParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  actor.code = kAccountCodeCid;
  // no pending txs in state
  MultiSignatureActorState actor_state{std::vector{address},
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_NOT_FOUND,
                       MultiSigActor::cancel(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor and pending tx with other creator
 * @when approve is called with address not equal to creator
 * @then error returned
 */
TEST_F(MultisigActorTest, CancelNotCreator) {
  BigInt actor_balance{100};
  BigInt value_to_send{50};
  actor.balance = actor_balance;
  actor.code = kAccountCodeCid;
  std::vector<Address> signers{address, kInitAddress};
  size_t threshold{2};
  TransactionNumber pending_tx_number{13};
  TransactionNumber tx_number{13};
  // tx is approved by kInitAddress - different creator
  MultiSignatureTransaction pending_tx{pending_tx_number,
                                       to_address,
                                       value_to_send,
                                       method_number,
                                       method_params,
                                       std::vector{kInitAddress}};
  MultiSignatureActorState actor_state{signers,
                                       threshold,
                                       tx_number,
                                       actor_balance,
                                       0,
                                       0,
                                       std::vector{pending_tx}};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));
  TransactionNumberParameters cancel_params{pending_tx_number};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(cancel_params));

  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));

  EXPECT_OUTCOME_ERROR(VMExitCode::MULTISIG_ACTOR_FORBIDDEN,
                       MultiSigActor::cancel(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor and pending tx
 * @when cancel is called
 * @then transaction is deleted from state
 */
TEST_F(MultisigActorTest, CancelSunnyDay) {
  BigInt actor_balance{100};
  BigInt value_to_send{50};
  actor.balance = actor_balance;
  actor.code = kAccountCodeCid;
  std::vector<Address> signers{address, kInitAddress};
  size_t threshold{2};
  TransactionNumber pending_tx_number{13};
  TransactionNumber tx_number{13};
  // tx is approved by caller - caller is creator
  MultiSignatureTransaction pending_tx{pending_tx_number,
                                       to_address,
                                       value_to_send,
                                       method_number,
                                       method_params,
                                       std::vector{address}};
  MultiSignatureActorState actor_state{signers,
                                       threshold,
                                       tx_number,
                                       actor_balance,
                                       0,
                                       0,
                                       std::vector{pending_tx}};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));
  TransactionNumberParameters approve_params{pending_tx_number};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(approve_params));

  // expected that pending tx is removed after cancel
  MultiSignatureActorState expected_state{
      signers,
      threshold,
      tx_number,
      actor_balance,
      0,
      0,
      std::vector<MultiSignatureTransaction>{}};

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(*datastore,
              set(_, MultiSignatureActorStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_EQ(MultiSigActor::cancel(actor, runtime, encoded_params),
                    InvocationOutput{});
}

/**
 * @given Runtime and multisig actor
 * @when addSigner() is called with immediate caller is not receiver
 * @then error WRONG_CALLER returned
 */
TEST_F(MultisigActorTest, AddSignerWrongCaller) {
  AddSignerParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver())
      .WillOnce(testing::Return(kInitAddress));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_WRONG_CALLER,
      MultiSigActor::addSigner(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when AddSigner() is called with address already is signer
 * @then error returned
 */
TEST_F(MultisigActorTest, AddSignerAlreadyAdded) {
  AddSignerParameters params{address};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  std::vector<Address> signers{address, kInitAddress};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT,
      MultiSigActor::addSigner(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when AddSigner() is called with not change threshold
 * @then new signer added, threshold is not changed
 */
TEST_F(MultisigActorTest, AddSignerNotChangeThreshold) {
  AddSignerParameters params{address, false};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  std::vector<Address> signers{kInitAddress};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  std::vector<Address> expected_signers{kInitAddress, address};
  MultiSignatureActorState expected_state{expected_signers,
                                          default_threshold,
                                          default_next_transaction_id,
                                          default_initial_balance,
                                          default_start_epoch,
                                          default_unlock_duration,
                                          default_pending_transactions};

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(*datastore,
              set(_, MultiSignatureActorStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_EQ(MultiSigActor::addSigner(actor, runtime, encoded_params),
                    InvocationOutput{});
}

/**
 * @given Runtime and multisig actor
 * @when AddSigner() is called with change threshold
 * @then new signer added, threshold is changed
 */
TEST_F(MultisigActorTest, AddSignerChangeThreshold) {
  AddSignerParameters params{address, true};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  std::vector<Address> signers{kInitAddress};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  std::vector<Address> expected_signers{kInitAddress, address};
  MultiSignatureActorState expected_state{expected_signers,
                                          default_threshold + 1,
                                          default_next_transaction_id,
                                          default_initial_balance,
                                          default_start_epoch,
                                          default_unlock_duration,
                                          default_pending_transactions};

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(*datastore,
              set(_, MultiSignatureActorStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_EQ(MultiSigActor::addSigner(actor, runtime, encoded_params),
                    InvocationOutput{});
}

/**
 * @given Runtime and multisig actor
 * @when removeSigner() is called with immediate caller is not receiver
 * @then error WRONG_CALLER returned
 */
TEST_F(MultisigActorTest, RemoveSignerWrongCaller) {
  RemoveSignerParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver())
      .WillOnce(testing::Return(kInitAddress));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_WRONG_CALLER,
      MultiSigActor::removeSigner(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when removeSigner() is called with address is not a signer
 * @then error returned
 */
TEST_F(MultisigActorTest, RemoveSignerNotAdded) {
  RemoveSignerParameters params{address};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  std::vector<Address> signers{kInitAddress};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_FORBIDDEN,
      MultiSigActor::removeSigner(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when removeSigner() is called with not change threshold
 * @then new signer added, threshold is not changed
 */
TEST_F(MultisigActorTest, RemoveSignerNotChangeThreshold) {
  RemoveSignerParameters params{address, false};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  std::vector<Address> signers{kInitAddress, address};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  std::vector<Address> expected_signers{kInitAddress};
  MultiSignatureActorState expected_state{expected_signers,
                                          default_threshold,
                                          default_next_transaction_id,
                                          default_initial_balance,
                                          default_start_epoch,
                                          default_unlock_duration,
                                          default_pending_transactions};

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(*datastore,
              set(_, MultiSignatureActorStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_EQ(MultiSigActor::removeSigner(actor, runtime, encoded_params),
                    InvocationOutput{});
}

/**
 * @given Runtime and multisig actor
 * @when removeSigner() is called with change threshold
 * @then new signer added, threshold is changed
 */
TEST_F(MultisigActorTest, RemoveSignerChangeThreshold) {
  AddSignerParameters params{address, true};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  std::vector<Address> signers{kInitAddress, address};
  size_t old_threshold{2};
  MultiSignatureActorState actor_state{signers,
                                       old_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  std::vector<Address> expected_signers{kInitAddress};
  MultiSignatureActorState expected_state{expected_signers,
                                          old_threshold - 1,
                                          default_next_transaction_id,
                                          default_initial_balance,
                                          default_start_epoch,
                                          default_unlock_duration,
                                          default_pending_transactions};

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(*datastore,
              set(_, MultiSignatureActorStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_EQ(MultiSigActor::removeSigner(actor, runtime, encoded_params),
                    InvocationOutput{});
}

/**
 * @given Runtime and multisig actor
 * @when removeSigner() is called with change threshold < 1
 * @then error returned
 */
TEST_F(MultisigActorTest, RemoveSignerChangeThresholdZero) {
  AddSignerParameters params{address, true};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  std::vector<Address> signers{kInitAddress, address};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT,
      MultiSigActor::removeSigner(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor and len(signers) == threshold
 * @when removeSigner() is called with not change threshold
 * @then error returned
 */
TEST_F(MultisigActorTest, RemoveSignerChangeThresholdError) {
  AddSignerParameters params{address, false};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  std::vector<Address> signers{kInitAddress, address};
  size_t old_threshold{2};
  MultiSignatureActorState actor_state{signers,
                                       old_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT,
      MultiSigActor::removeSigner(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when swapSigner() is called with immediate caller is not receiver
 * @then error WRONG_CALLER returned
 */
TEST_F(MultisigActorTest, SwapSignerWrongCaller) {
  SwapSignerParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver())
      .WillOnce(testing::Return(kInitAddress));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_WRONG_CALLER,
      MultiSigActor::swapSigner(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when swapSigner() is called with address is not a signer
 * @then error returned
 */
TEST_F(MultisigActorTest, SwapSignerNotAdded) {
  SwapSignerParameters params{address, kCronAddress};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  // old signer not present
  std::vector<Address> signers{kInitAddress};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_NOT_FOUND,
      MultiSigActor::swapSigner(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when swapSigner() is called with new address is already a signer
 * @then error returned
 */
TEST_F(MultisigActorTest, SwapSignerAlreadyAdded) {
  SwapSignerParameters params{address, kCronAddress};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  // new signer is already present
  std::vector<Address> signers{address, kCronAddress};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT,
      MultiSigActor::swapSigner(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when swapSigner() is called
 * @then state updated and sucess returned
 */
TEST_F(MultisigActorTest, SwapSignerSuccess) {
  SwapSignerParameters params{address, kCronAddress};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  std::vector<Address> signers{address, kInitAddress};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state));

  std::vector<Address> expected_signers{kCronAddress, kInitAddress};
  MultiSignatureActorState expected_state{expected_signers,
                                          default_threshold,
                                          default_next_transaction_id,
                                          default_initial_balance,
                                          default_start_epoch,
                                          default_unlock_duration,
                                          default_pending_transactions};

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(*datastore,
              set(_, MultiSignatureActorStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_EQ(MultiSigActor::swapSigner(actor, runtime, encoded_params),
                    InvocationOutput{});
}

/**
 * @given Runtime and multisig actor
 * @when changeThreshold() is called with immediate caller is not receiver
 * @then error WRONG_CALLER returned
 */
TEST_F(MultisigActorTest, ChangeThresholdWrongCaller) {
  ChangeThresholdParameters params{};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver())
      .WillOnce(testing::Return(kInitAddress));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_WRONG_CALLER,
      MultiSigActor::changeThreshold(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when changeThreshold() is called with 0 threshold
 * @then error returned
 */
TEST_F(MultisigActorTest, ChangeThresholdZero) {
  ChangeThresholdParameters params{0};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  std::vector<Address> signers{address, kInitAddress};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT,
      MultiSigActor::changeThreshold(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when changeThreshold() is called with threshold more than number of signers
 * @then error returned
 */
TEST_F(MultisigActorTest, ChangeThresholdMoreThanSigners) {
  ChangeThresholdParameters params{100500};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));

  std::vector<Address> signers{address, kInitAddress};
  MultiSignatureActorState actor_state{signers,
                                       default_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore()).WillOnce(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MULTISIG_ACTOR_ILLEGAL_ARGUMENT,
      MultiSigActor::changeThreshold(actor, runtime, encoded_params));
}

/**
 * @given Runtime and multisig actor
 * @when changeThreshold() is called with new threshold
 * @then new threshold saved to actor state
 */
TEST_F(MultisigActorTest, ChangeThresholdSuccess) {
  size_t old_threshold = 1;
  size_t new_threshold = 2;
  ChangeThresholdParameters params{new_threshold};
  EXPECT_OUTCOME_TRUE(encoded_params, encodeActorParams(params));
  std::vector<Address> signers{address, kInitAddress};
  MultiSignatureActorState actor_state{signers,
                                       old_threshold,
                                       default_next_transaction_id,
                                       default_initial_balance,
                                       default_start_epoch,
                                       default_unlock_duration,
                                       default_pending_transactions};
  EXPECT_OUTCOME_TRUE(encoded_state, fc::codec::cbor::encode(actor_state))

  MultiSignatureActorState expected_state{signers,
                                          new_threshold,
                                          default_next_transaction_id,
                                          default_initial_balance,
                                          default_start_epoch,
                                          default_unlock_duration,
                                          default_pending_transactions};

  EXPECT_CALL(runtime, getImmediateCaller()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getCurrentReceiver()).WillOnce(testing::Return(address));
  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(datastore));
  EXPECT_CALL(*datastore, get(_))
      .WillOnce(testing::Return(fc::outcome::success(encoded_state)));
  EXPECT_CALL(*datastore,
              set(_, MultiSignatureActorStateMatcher(expected_state)))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(runtime, commit(_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_EQ(
      MultiSigActor::changeThreshold(actor, runtime, encoded_params),
      InvocationOutput{});
}
