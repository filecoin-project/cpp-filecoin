/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/states/multisig/v3/multisig_actor_state.hpp"

#include <gtest/gtest.h>
#include "primitives/address/address.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/codes.hpp"
#include "vm/state/impl/state_tree_impl.hpp"
#include "vm/version/version.hpp"

#define ON_CALL_3(object, call, result) \
  EXPECT_CALL(object, call).WillRepeatedly(Return(result))

namespace fc::vm::actor::builtin::v3::multisig {
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::InvocationOutput;
  using runtime::MockRuntime;
  using state::StateTreeImpl;

  using storage::ipfs::InMemoryDatastore;
  using testing::Eq;
  using testing::Return;
  using types::multisig::Transaction;
  using types::multisig::TransactionId;
  using version::NetworkVersion;

  class MultisigActorTest : public ::testing::Test {
    void SetUp() override {
      initState();
      actor_version = ActorVersion::kVersion3;
      ipld->actor_version = actor_version;

      EXPECT_CALL(runtime, getActorVersion())
          .WillRepeatedly(testing::Invoke([&]() { return actor_version; }));

      ON_CALL_3(runtime, getIpfsDatastore(), ipld);

      runtime.resolveAddressWith(state_tree);

      EXPECT_CALL(runtime, getCurrentEpoch())
          .WillRepeatedly(testing::Invoke([&]() { return epoch; }));

      ON_CALL_3(runtime, getValueReceived(), value_received);

      EXPECT_CALL(runtime, getNetworkVersion())
          .Times(testing::AtMost(1))
          .WillOnce(testing::Invoke(
              [&]() { return version::getNetworkVersion(epoch); }));

      EXPECT_CALL(runtime, getBalance(actor_address))
          .WillRepeatedly(testing::Invoke(
              [&](auto &) { return fc::outcome::success(balance); }));

      EXPECT_CALL(runtime, getImmediateCaller())
          .WillRepeatedly(testing::Invoke([&]() { return caller; }));

      ON_CALL_3(runtime, getCurrentReceiver(), actor_address);

      ON_CALL_3(runtime, getActorCodeID(kInitAddress), kInitCodeId);
      ON_CALL_3(runtime, getActorCodeID(caller), kAccountCodeId);
      ON_CALL_3(runtime, getActorCodeID(wrong_caller), kCronCodeId);

      EXPECT_CALL(runtime, hashBlake2b(testing::_))
          .WillRepeatedly(testing::Invoke(
              [&](auto &data) { return crypto::blake2b::blake2b_256(data); }));

      EXPECT_CALL(runtime, commit(testing::_))
          .WillRepeatedly(testing::Invoke([&](auto &cid) {
            EXPECT_OUTCOME_TRUE(new_state,
                                getCbor<MultisigActorState>(ipld, cid));
            state = std::move(new_state);
            return outcome::success();
          }));

      EXPECT_CALL(runtime, getActorStateCid())
          .WillRepeatedly(testing::Invoke([&]() {
            EXPECT_OUTCOME_TRUE(cid, setCbor(ipld, state));
            return std::move(cid);
          }));
    }

   public:
    void resetSigners() {
      state.signers.clear();
      pushSigner(caller);
    }

    void pushSigner(const Address &signer) {
      state.signers.push_back(resolveAddr(signer));
    }

    Address resolveAddr(const Address &address) {
      EXPECT_OUTCOME_TRUE(resolved, state_tree.lookupId(address));
      return std::move(resolved);
    }

    void initState() {
      pushSigner(caller);
      state.threshold = 1;
      state.next_transaction_id = 1;
      state.initial_balance = 0;
      state.start_epoch = 0;
      state.unlock_duration = 0;

      cbor_blake::cbLoadT(ipld, state);
    }

   protected:
    /** Immediate caller address */
    Address caller{Address::makeFromId(101)};
    Address wrong_caller{Address::makeFromId(999)};

    /** Pendign transaction receiver */
    Address to_address{Address::makeFromId(102)};

    /** Multisig actor address */
    Address actor_address{Address::makeFromId(103)};

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};

    MethodNumber method_number{1};
    MethodParams method_params{"0102"_unhex};

    ChainEpoch epoch;

    TokenAmount balance;
    BigInt value_received;

    MultisigActorState state{};

    StateTreeImpl state_tree{ipld};
    ActorVersion actor_version;
  };

  /**
   * @given Runtime and multisig actor
   * @when constructor is called with immediate caller different from Init Actor
   * @then error returned
   */
  TEST_F(MultisigActorTest, ConstructWrongCaller) {
    caller = wrong_caller;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         Construct::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when constructor is called with empty signers
   * @then error returned
   */
  TEST_F(MultisigActorTest, ConstructEmptySigners) {
    caller = kInitAddress;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         Construct::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when constructor is called with empty signers
   * @then error returned
   */
  TEST_F(MultisigActorTest, ConstructMaxSigners) {
    caller = kInitAddress;
    std::vector<Address> signers;
    signers.resize(256, Address{});
    const size_t threshold{2};

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        Construct::call(runtime, {signers, threshold, {}, {}}));
  }

  /**
   * @given Runtime and multisig actor
   * @when constructor is called with duplicate signers
   * @then error returned
   */
  TEST_F(MultisigActorTest, ConstructDuplicateSigners) {
    caller = kInitAddress;
    const std::vector<Address> signers{caller, caller};
    const size_t threshold{2};

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        Construct::call(runtime, {signers, threshold, {}, {}}));
  }

  /**
   * @given Runtime and multisig actor
   * @when constructor is called with threshold more than signers
   * @then error returned
   */
  TEST_F(MultisigActorTest, ConstructWrongThreshold) {
    caller = kInitAddress;
    const std::vector<Address> signers{caller};
    const size_t threshold{5};

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        Construct::call(runtime, {signers, threshold, {}, {}}));
  }

  /**
   * @given Runtime and multisig actor
   * @when constructor is called with 0 threshold
   * @then error returned
   */
  TEST_F(MultisigActorTest, ConstructZeroThreshold) {
    caller = kInitAddress;
    const std::vector<Address> signers{caller};
    const size_t threshold{0};

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        Construct::call(runtime, {signers, threshold, {}, {}}));
  }

  /**
   * @given Runtime and multisig actor
   * @when constructor is called with unlock_duration < 0
   * @then error returned
   */
  TEST_F(MultisigActorTest, ConstructNegativeDuration) {
    caller = kInitAddress;
    const std::vector<Address> signers{caller};
    const size_t threshold{1};
    const EpochDuration duration = -1;

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        Construct::call(runtime, {signers, threshold, duration, {}}));
  }

  /**
   * @given Runtime and multisig actor
   * @when constructor is called with correct parameters
   * @then success returned and state is commited to storage
   */
  TEST_F(MultisigActorTest, ConstrucCorrect) {
    caller = kInitAddress;
    ChainEpoch start_epoch = 42;

    const std::vector<Address> signers{caller};
    const size_t threshold{1};

    EXPECT_OUTCOME_TRUE_1(
        Construct::call(runtime, {signers, threshold, start_epoch}));
  }

  /**
   * @given Runtime and multisig actor
   * @when propose is called with immediate caller is not signable
   * @then error returned
   */
  TEST_F(MultisigActorTest, ProposeWrongCaller) {
    caller = wrong_caller;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         Propose::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when propose is called with not signer
   * @then error returned
   */
  TEST_F(MultisigActorTest, ProposetWrongSigner) {
    state.signers.clear();

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         Propose::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor and no funds and no lock period
   * @when propose is called with threshold 1 and value transferred
   * @then error insufficient funds
   */
  TEST_F(MultisigActorTest, ProposeSendInsufficientFunds) {
    balance = 1;

    const BigInt value_to_send{100500};  // > actor state balance

    state.next_transaction_id = 13;
    state.initial_balance = balance;

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrInsufficientFunds),
        Propose::call(
            runtime,
            {to_address, value_to_send, method_number, method_params}));
  }

  /**
   * @given Runtime and multisig actor and funds locked
   * @when propose is called with threshold 1 and value transferred
   * @then error funds locked
   */
  TEST_F(MultisigActorTest, ProposeSendFundsLocked) {
    balance = 200;
    epoch = 43;

    const BigInt value_to_send{200};

    state.next_transaction_id = 13;
    state.initial_balance = balance;
    state.start_epoch = 42;
    state.unlock_duration = 10;

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrInsufficientFunds),
        Propose::call(
            runtime,
            {to_address, value_to_send, method_number, method_params}));
  }

  /**
   * @given Runtime and multisig actor and funds locked because start epoch more
   * than current epoch
   * @when propose is called with threshold 1 and value transferred
   * @then error funds locked
   */
  TEST_F(MultisigActorTest, ProposeSendFundsLockedStartEpoch) {
    balance = 200;
    epoch = 10;

    const BigInt value_to_send{200};

    state.next_transaction_id = 13;
    state.initial_balance = balance;
    state.start_epoch = 42;
    state.unlock_duration = 10;

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrInsufficientFunds),
        Propose::call(
            runtime,
            {to_address, value_to_send, method_number, method_params}));
  }

  /**
   * @given Runtime and multisig actor
   * @when propose is called with threshold 1
   * @then transaction is sent
   */
  TEST_F(MultisigActorTest, ProposeSendFundsEnough) {
    balance = 100;
    epoch = 42;

    const TransactionId tx_id{13};
    const BigInt value_to_send{50};

    state.next_transaction_id = tx_id;
    state.initial_balance = balance;

    auto expected_state = state;
    expected_state.next_transaction_id = tx_id + 1;

    EXPECT_CALL(runtime,
                send(Eq(to_address),
                     Eq(method_number),
                     Eq(method_params),
                     Eq(value_to_send)))
        .WillOnce(testing::Return(fc::outcome::success(InvocationOutput{})));

    const Propose::Result expected_result{tx_id, true, VMExitCode::kOk, {}};
    EXPECT_OUTCOME_EQ(
        Propose::call(
            runtime, {to_address, value_to_send, method_number, method_params}),
        expected_result);
  }

  /**
   * @given Runtime and multisig actor
   * @when propose is called with threshold 2
   * @then transaction is pending
   */
  TEST_F(MultisigActorTest, ProposePending) {
    balance = 100;

    const TransactionId tx_id{13};
    const BigInt value_to_send{50};

    state.threshold = 2;
    state.next_transaction_id = tx_id;
    state.initial_balance = balance;

    const Transaction pending_tx{
        to_address, value_to_send, method_number, method_params, {caller}};

    const auto expected_tx_id = tx_id + 1;
    const Propose::Result expected_result{tx_id, false, VMExitCode::kOk, {}};

    EXPECT_OUTCOME_EQ(
        Propose::call(
            runtime, {to_address, value_to_send, method_number, method_params}),
        expected_result);

    EXPECT_EQ(state.next_transaction_id, expected_tx_id);
    EXPECT_OUTCOME_TRUE(tx, state.pending_transactions.get(tx_id));
    EXPECT_EQ(tx, pending_tx);
  }

  /**
   * @given Runtime and multisig actor
   * @when approve is called with immediate caller is not signable
   * @then error returned
   */
  TEST_F(MultisigActorTest, ApproveWrongCaller) {
    caller = wrong_caller;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         Approve::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when approve is called with wrong signer
   * @then error returned
   */
  TEST_F(MultisigActorTest, ApproveWrongSigner) {
    state.signers.clear();
    pushSigner(kInitAddress);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         Approve::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when approve is called with wrong tx_id
   * @then error returned
   */
  TEST_F(MultisigActorTest, ApproveWrongTxId) {
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrNotFound),
                         Approve::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor and pending tx
   * @when approve is called with caller already signed
   * @then error returned
   */
  TEST_F(MultisigActorTest, ApproveAlreadySigned) {
    const TransactionId tx_id{13};
    const BigInt value_to_send{50};

    // tx is approved by caller
    const Transaction pending_tx{
        to_address, value_to_send, method_number, method_params, {caller}};

    pushSigner(kInitAddress);
    state.threshold = 2;
    state.next_transaction_id = tx_id;
    state.initial_balance = 100;
    EXPECT_OUTCOME_TRUE_1(state.pending_transactions.set(tx_id, pending_tx));

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         Approve::call(runtime, {tx_id, {}}));
  }

  /**
   * @given Runtime and multisig actor and pending tx
   * @when approve is called with wron proposal hash
   * @then error returned
   */
  TEST_F(MultisigActorTest, ApproveWrongHash) {
    balance = 100;
    epoch = 42;

    const TransactionId tx_id{13};
    const BigInt value_to_send{50};

    // tx is approved by kInitAddress
    const Transaction pending_tx{to_address,
                                 value_to_send,
                                 method_number,
                                 method_params,
                                 {kInitAddress}};

    const Bytes wrong_hash{"010203"_unhex};

    pushSigner(kInitAddress);
    state.threshold = 2;
    state.next_transaction_id = tx_id;
    state.initial_balance = balance;
    EXPECT_OUTCOME_TRUE_1(state.pending_transactions.set(tx_id, pending_tx));

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         Approve::call(runtime, {tx_id, wrong_hash}));
  }

  /**
   * @given Runtime and multisig actor and pending tx
   * @when approve is called
   * @then transaction is signed, called and deleted from state
   */
  TEST_F(MultisigActorTest, ApproveSuccess) {
    balance = 100;
    epoch = 42;

    const TransactionId tx_id{13};
    const BigInt value_to_send{50};

    // tx is approved by kInitAddress
    const Transaction pending_tx{to_address,
                                 value_to_send,
                                 method_number,
                                 method_params,
                                 {kInitAddress}};

    EXPECT_OUTCOME_TRUE(hash, pending_tx.hash(runtime));

    pushSigner(kInitAddress);
    state.threshold = 2;
    state.next_transaction_id = tx_id;
    state.initial_balance = balance;
    EXPECT_OUTCOME_TRUE_1(state.pending_transactions.set(tx_id, pending_tx));

    EXPECT_CALL(runtime,
                send(Eq(to_address),
                     Eq(method_number),
                     Eq(method_params),
                     Eq(value_to_send)))
        .WillOnce(testing::Return(fc::outcome::success(InvocationOutput{})));

    EXPECT_OUTCOME_TRUE_1(Approve::call(runtime, {tx_id, hash}));

    // expected that pending tx is removed after sending
    EXPECT_OUTCOME_TRUE(exist, state.pending_transactions.has(tx_id));
    EXPECT_EQ(exist, false);
  }

  /**
   * @given Runtime and multisig actor
   * @when cancel is called with immediate caller is not signable
   * @then error returned
   */
  TEST_F(MultisigActorTest, CancelWrongCaller) {
    caller = wrong_caller;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         Cancel::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when cancel is called with wrong signer
   * @then error returned
   */
  TEST_F(MultisigActorTest, CancelWrongSigner) {
    state.signers.clear();
    pushSigner(kInitAddress);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         Cancel::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when cancel is called with wrong tx_id
   * @then error returned
   */
  TEST_F(MultisigActorTest, CancelWrongTxId) {
    // no pending txs in state
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrNotFound),
                         Cancel::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor and pending tx with other creator
   * @when approve is called with approve address not equal to caller
   * @then error returned
   */
  TEST_F(MultisigActorTest, CancelApproverNotCaller) {
    const TransactionId tx_id{13};

    // tx is approved by kInitAddress - different creator
    const Transaction pending_tx{
        to_address, 50, method_number, method_params, {kInitAddress}};

    pushSigner(kInitAddress);
    state.threshold = 2;
    state.next_transaction_id = tx_id;
    state.initial_balance = 100;
    EXPECT_OUTCOME_TRUE_1(state.pending_transactions.set(tx_id, pending_tx));

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         Cancel::call(runtime, {tx_id, {}}));
  }

  /**
   * @given Runtime and multisig actor and pending tx
   * @when cancel is called with wrong proposal hash
   * @then error returned
   */
  TEST_F(MultisigActorTest, CancelWrongHash) {
    const TransactionId tx_id{13};

    // tx is approved by caller - caller is creator
    const Transaction pending_tx{
        to_address, 50, method_number, method_params, {caller}};

    pushSigner(kInitAddress);
    state.threshold = 2;
    state.next_transaction_id = tx_id;
    state.initial_balance = 100;
    EXPECT_OUTCOME_TRUE_1(state.pending_transactions.set(tx_id, pending_tx));

    const Bytes wrong_hash{"010203"_unhex};

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalState),
                         Cancel::call(runtime, {tx_id, wrong_hash}));
  }

  /**
   * @given Runtime and multisig actor and pending tx
   * @when cancel is called
   * @then transaction is deleted from state
   */
  TEST_F(MultisigActorTest, CancelSuccess) {
    const TransactionId tx_id{13};

    // tx is approved by caller - caller is creator
    const Transaction pending_tx{
        to_address, 50, method_number, method_params, {caller}};

    pushSigner(kInitAddress);
    state.threshold = 2;
    state.next_transaction_id = tx_id;
    state.initial_balance = 100;
    EXPECT_OUTCOME_TRUE_1(state.pending_transactions.set(tx_id, pending_tx));

    EXPECT_OUTCOME_TRUE(hash, pending_tx.hash(runtime));

    EXPECT_OUTCOME_TRUE_1(Cancel::call(runtime, {tx_id, hash}));

    // expected that pending tx is removed after sending
    EXPECT_OUTCOME_TRUE(exist, state.pending_transactions.has(tx_id));
    EXPECT_EQ(exist, false);
  }

  /**
   * @given Runtime and multisig actor
   * @when addSigner is called with immediate caller is not receiver
   * @then error returned
   */
  TEST_F(MultisigActorTest, AddSignerWrongCaller) {
    caller = wrong_caller;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         AddSigner::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when addSigner is called state already contains max count of signers
   * @then error returned
   */
  TEST_F(MultisigActorTest, AddSignerMaxSigners) {
    caller = actor_address;
    resetSigners();
    state.signers.resize(256, Address{});
    pushSigner(kInitAddress);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         AddSigner::call(runtime, {caller}));
  }

  /**
   * @given Runtime and multisig actor
   * @when addSigner is called with address already is signer
   * @then error returned
   */
  TEST_F(MultisigActorTest, AddSignerAlreadyAdded) {
    caller = actor_address;
    resetSigners();
    pushSigner(kInitAddress);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         AddSigner::call(runtime, {caller}));
  }

  /**
   * @given Runtime and multisig actor
   * @when addSigner is called with not change threshold
   * @then new signer added, threshold is not changed
   */
  TEST_F(MultisigActorTest, AddSignerNotChangeThreshold) {
    caller = actor_address;
    state.signers.clear();
    pushSigner(kInitAddress);

    const std::vector<Address> expected_signers{resolveAddr(kInitAddress),
                                                resolveAddr(caller)};
    const auto expected_threshold = state.threshold;

    EXPECT_OUTCOME_TRUE_1(AddSigner::call(runtime, {caller, false}));

    EXPECT_EQ(state.signers, expected_signers);
    EXPECT_EQ(state.threshold, expected_threshold);
  }

  /**
   * @given Runtime and multisig actor
   * @when addSigner is called with change threshold
   * @then new signer added, threshold is changed
   */
  TEST_F(MultisigActorTest, AddSignerChangeThreshold) {
    caller = actor_address;
    state.signers.clear();
    pushSigner(kInitAddress);

    const std::vector<Address> expected_signers{resolveAddr(kInitAddress),
                                                resolveAddr(caller)};
    const auto expected_threshold = state.threshold + 1;

    EXPECT_OUTCOME_TRUE_1(AddSigner::call(runtime, {caller, true}));

    EXPECT_EQ(state.signers, expected_signers);
    EXPECT_EQ(state.threshold, expected_threshold);
  }

  /**
   * @given Runtime and multisig actor
   * @when removeSigner is called with immediate caller is not receiver
   * @then error returned
   */
  TEST_F(MultisigActorTest, RemoveSignerWrongCaller) {
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         RemoveSigner::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when removeSigner() is called with address is not a signer
   * @then error returned
   */
  TEST_F(MultisigActorTest, RemoveSignerNotAdded) {
    caller = actor_address;

    state.signers.clear();
    pushSigner(kInitAddress);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         RemoveSigner::call(runtime, {caller}));
  }

  /**
   * @given Runtime and multisig actor
   * @when removeSigner is called with not change threshold
   * @then new signer added, threshold is not changed
   */
  TEST_F(MultisigActorTest, RemoveSignerNotChangeThreshold) {
    caller = actor_address;
    resetSigners();
    pushSigner(kInitAddress);

    const std::vector<Address> expected_signers{resolveAddr(kInitAddress)};
    const auto expected_threshold = state.threshold;

    EXPECT_OUTCOME_TRUE_1(RemoveSigner::call(runtime, {caller, false}));

    EXPECT_EQ(state.signers, expected_signers);
    EXPECT_EQ(state.threshold, expected_threshold);
  }

  /**
   * @given Runtime and multisig actor
   * @when removeSigner is called with change threshold
   * @then new signer added, threshold is changed
   */
  TEST_F(MultisigActorTest, RemoveSignerChangeThreshold) {
    caller = actor_address;
    resetSigners();
    pushSigner(kInitAddress);
    state.threshold = 2;

    const std::vector<Address> expected_signers{resolveAddr(kInitAddress)};
    const auto expected_threshold = state.threshold - 1;

    EXPECT_OUTCOME_TRUE_1(RemoveSigner::call(runtime, {caller, true}));

    EXPECT_EQ(state.signers, expected_signers);
    EXPECT_EQ(state.threshold, expected_threshold);
  }

  /**
   * @given Runtime and multisig actor
   * @when removeSigner is called with change threshold more than signers
   * after removing
   * @then error returned
   */
  TEST_F(MultisigActorTest, RemoveSignerChangeThresholdMoreThanSigners) {
    caller = actor_address;
    resetSigners();
    pushSigner(kInitAddress);
    state.threshold = 5;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         RemoveSigner::call(runtime, {caller, false}));
  }

  /**
   * @given Runtime and multisig actor and len(signers) == threshold
   * @when removeSigner is called with not change threshold
   * @then error returned
   */
  TEST_F(MultisigActorTest, RemoveSignerChangeThresholdError) {
    caller = actor_address;
    resetSigners();
    pushSigner(kInitAddress);
    state.threshold = 2;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         RemoveSigner::call(runtime, {caller, false}));
  }

  /**
   * @given Runtime and multisig actor
   * @when swapSigner is called with immediate caller is not receiver
   * @then error returned
   */
  TEST_F(MultisigActorTest, SwapSignerWrongCaller) {
    caller = wrong_caller;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         SwapSigner::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when swapSigner is called with address is not a signer
   * @then error returned
   */
  TEST_F(MultisigActorTest, SwapSignerNotAdded) {
    caller = actor_address;

    state.signers.clear();
    // old signer not present
    pushSigner(kInitAddress);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         SwapSigner::call(runtime, {caller, kCronAddress}));
  }

  /**
   * @given Runtime and multisig actor
   * @when swapSigner is called with new address is already a signer
   * @then error returned
   */
  TEST_F(MultisigActorTest, SwapSignerAlreadyAdded) {
    caller = actor_address;
    resetSigners();

    // new signer is already present
    pushSigner(kCronAddress);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         SwapSigner::call(runtime, {caller, kCronAddress}));
  }

  /**
   * @given Runtime and multisig actor
   * @when swapSigner is called
   * @then state updated and sucess returned
   */
  TEST_F(MultisigActorTest, SwapSignerSuccess) {
    caller = actor_address;
    resetSigners();
    pushSigner(kInitAddress);

    const std::vector<Address> expected_signers{
        resolveAddr(kInitAddress),
        resolveAddr(kCronAddress),
    };

    EXPECT_OUTCOME_TRUE_1(SwapSigner::call(runtime, {caller, kCronAddress}));

    EXPECT_EQ(state.signers, expected_signers);
  }

  /**
   * @given Runtime and multisig actor
   * @when changeThreshold is called with immediate caller is not receiver
   * @then error returned
   */
  TEST_F(MultisigActorTest, ChangeThresholdWrongCaller) {
    caller = wrong_caller;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         ChangeThreshold::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when changeThreshold is called with 0 threshold
   * @then error returned
   */
  TEST_F(MultisigActorTest, ChangeThresholdZero) {
    caller = actor_address;
    resetSigners();
    pushSigner(kInitAddress);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         ChangeThreshold::call(runtime, {0}));
  }

  /**
   * @given Runtime and multisig actor
   * @when changeThreshold is called with threshold more than number of
   * signers
   * @then error returned
   */
  TEST_F(MultisigActorTest, ChangeThresholdMoreThanSigners) {
    caller = actor_address;

    pushSigner(kInitAddress);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         ChangeThreshold::call(runtime, {100500}));
  }

  /**
   * @given Runtime and multisig actor
   * @when changeThreshold is called with new threshold
   * @then new threshold saved to actor state
   */
  TEST_F(MultisigActorTest, ChangeThresholdSuccess) {
    caller = actor_address;
    resetSigners();
    pushSigner(kInitAddress);

    const size_t new_threshold = 2;

    EXPECT_OUTCOME_TRUE_1(ChangeThreshold::call(runtime, {new_threshold}));

    EXPECT_EQ(state.threshold, new_threshold);
  }

  /**
   * @given Runtime and multisig actor
   * @when lockBalance is called with immediate caller is not receiver
   * @then error returned
   */
  TEST_F(MultisigActorTest, LockBalanceWrongCaller) {
    caller = wrong_caller;
    epoch = 272401;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         LockBalance::call(runtime, {}));
  }

  /**
   * @given Runtime and multisig actor
   * @when lockBalance is called with unlock_duration <= 0
   * @then error returned
   */
  TEST_F(MultisigActorTest, LockBalanceWrongDuration) {
    caller = actor_address;
    epoch = 272401;

    const ChainEpoch start_epoch = 42;
    const EpochDuration unlock_duration = 0;
    const TokenAmount amount = 100;

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        LockBalance::call(runtime, {start_epoch, unlock_duration, amount}));
  }

  /**
   * @given Runtime and multisig actor
   * @when lockBalance is called with epoch > network version 2 and negative
   * amount
   * @then error returned
   */
  TEST_F(MultisigActorTest, LockBalanceWrongAmount) {
    caller = actor_address;
    epoch = 272401;

    const ChainEpoch start_epoch = 42;
    const EpochDuration unlock_duration = 1;
    const TokenAmount amount = -1;

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        LockBalance::call(runtime, {start_epoch, unlock_duration, amount}));
  }

  /**
   * @given Runtime and multisig actor
   * @when lockBalance is called
   * @then balance successfully locked
   */
  TEST_F(MultisigActorTest, LockBalanceSuccess) {
    caller = actor_address;
    epoch = 272401;

    const ChainEpoch start_epoch = 42;
    const EpochDuration unlock_duration = 3;
    const TokenAmount amount = 100;

    EXPECT_OUTCOME_TRUE_1(
        LockBalance::call(runtime, {start_epoch, unlock_duration, amount}));

    EXPECT_EQ(state.start_epoch, start_epoch);
    EXPECT_EQ(state.unlock_duration, unlock_duration);
    EXPECT_EQ(state.initial_balance, amount);
  }

}  // namespace fc::vm::actor::builtin::v3::multisig
