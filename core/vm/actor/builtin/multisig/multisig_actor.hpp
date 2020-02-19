/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_MULTISIG_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_MULTISIG_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/cid/cid.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::multisig {

  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::address::Address;
  using runtime::InvocationOutput;
  using runtime::Runtime;
  using TransactionNumber = size_t;

  constexpr MethodNumber kProposeMethodNumber{2};
  constexpr MethodNumber kApproveMethodNumber{3};
  constexpr MethodNumber kCancelMethodNumber{4};
  constexpr MethodNumber kAddSignerMethodNumber{5};
  constexpr MethodNumber kRemoveSignerMethodNumber{6};
  constexpr MethodNumber kSwapSignerMethodNumber{7};
  constexpr MethodNumber kChangeThresholdMethodNumber{7};

  /**
   * Multisignaure pending transaction
   */
  struct MultiSignatureTransaction {
    /** Transaction id given by Multisignature Actor */
    TransactionNumber transaction_number;
    Address to;
    BigInt value;
    MethodNumber method;
    MethodParams params;

    /**
     * @brief List of addresses that approved transaction
     * This address at index 0 is the transaction proposer, order of this slice
     * must be preserved
     */
    std::vector<Address> approved;

    bool operator==(const MultiSignatureTransaction &other) const;
  };

  /**
   * State of Multisig Actor instance
   */
  struct MultiSignatureActorState {
    std::vector<Address> signers;
    size_t threshold;
    /** Transaction counter */
    TransactionNumber next_transaction_id;

    // Linear lock
    BigInt initial_balance;
    ChainEpoch start_epoch;
    EpochDuration unlock_duration;

    /** List of pending transactions */
    std::vector<MultiSignatureTransaction> pending_transactions;

    /**
     * Checks if address is signer
     */
    bool isSigner(const Address &address) const;

    /**
     * Checks if address is a creator of transaction
     * @param tx_number - number of transaction to check
     * @param address - address to check
     * @return true if address proposed the transaction
     */
    outcome::result<bool> isTransactionCreator(
        const TransactionNumber &tx_number, const Address &address) const;

    /**
     * Get pending transaction
     */
    outcome::result<MultiSignatureTransaction> getPendingTransaction(
        const TransactionNumber &tx_number) const;

    /**
     * Update pending transaction by transaction_number
     * @param transaction - to update
     * @return nothing or error occurred
     */
    outcome::result<void> updatePendingTransaction(
        const MultiSignatureTransaction &transaction);

    /**
     * Delete pending transaction by tx_number
     * @param tx_number - to delete
     * @return nothing or error occurred
     */
    outcome::result<void> deletePendingTransaction(
        const TransactionNumber &tx_number);

    /**
     * Approve pending transaction by tx_number. Add signer and if threshold is
     * met send pending tx
     * @param actor - caller
     * @param runtime - execution context
     * @param tx_number - tx to approve
     * @return nothing or error occurred
     */
    outcome::result<void> approveTransaction(
        const Actor &actor,
        Runtime &runtime,
        const TransactionNumber &tx_number);

    /**
     * Get amount locked for current epoch
     * @param current_epoch - current block number
     * @return - return amount locked
     */
    BigInt getAmountLocked(const ChainEpoch &current_epoch) const;
  };

  /**
   * Construct method parameters
   */
  struct ConstructParameters {
    std::vector<Address> signers;
    size_t threshold;
    EpochDuration unlock_duration;
  };

  /**
   * Propose method parameters
   */
  struct ProposeParameters {
    Address to;
    BigInt value;
    MethodNumber method{};
    MethodParams params;
  };

  /**
   * TransactionNumber method parameters for approve and cancel
   */
  struct TransactionNumberParameters {
    TransactionNumber transaction_number;
  };

  /**
   * AddSigner method parameters for approve and cancel
   */
  struct AddSignerParameters {
    Address signer;
    bool increase_threshold{false};
  };

  /**
   * RemoveSigner method parameters for approve and cancel
   */
  struct RemoveSignerParameters {
    Address signer;
    bool decrease_threshold{false};
  };

  /**
   * RemoveSigner method parameters for approve and cancel
   */
  struct SwapSignerParameters {
    Address old_signer;
    Address new_signer;
  };

  /**
   * ChangeThreshold method parameters for approve and cancel
   */
  struct ChangeThresholdParameters {
    size_t new_threshold;
  };

  /**
   * Multisignature actor methods
   */
  class MultiSigActor {
   public:
    static outcome::result<InvocationOutput> construct(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> propose(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> approve(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> cancel(const Actor &actor,
                                                    Runtime &runtime,
                                                    const MethodParams &params);

    static outcome::result<InvocationOutput> addSigner(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> removeSigner(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> swapSigner(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> changeThreshold(
        const Actor &actor, Runtime &runtime, const MethodParams &params);
  };

  /** Exported Multisig Actor methods to invoker */
  extern const ActorExports exports;

  /**
   * CBOR serialization of MultiSignatureTransaction
   */
  CBOR_ENCODE(MultiSignatureTransaction, transaction) {
    return s << (s.list() << transaction.transaction_number << transaction.to
                          << transaction.value << transaction.method
                          << transaction.params << transaction.approved);
  }

  /**
   * CBOR deserialization of MultiSignatureTransaction
   */
  CBOR_DECODE(MultiSignatureTransaction, transaction) {
    s.list() >> transaction.transaction_number >> transaction.to
        >> transaction.value >> transaction.method >> transaction.params
        >> transaction.approved;
    return s;
  }

  /**
   * CBOR serialization of MultiSignatureActorState
   */
  CBOR_ENCODE(MultiSignatureActorState, state) {
    return s << (s.list() << state.signers << state.threshold
                          << state.next_transaction_id << state.initial_balance
                          << state.start_epoch << state.unlock_duration
                          << state.pending_transactions);
  }

  /**
   * CBOR deserialization of MultiSignatureActorState
   */
  CBOR_DECODE(MultiSignatureActorState, state) {
    s.list() >> state.signers >> state.threshold >> state.next_transaction_id
        >> state.initial_balance >> state.start_epoch >> state.unlock_duration
        >> state.pending_transactions;
    return s;
  }

  /**
   * CBOR serialization of ConstructParameters
   */
  CBOR_ENCODE(ConstructParameters, construct_params) {
    return s << (s.list() << construct_params.signers
                          << construct_params.threshold
                          << construct_params.unlock_duration);
  }

  /**
   * CBOR deserialization of ConstructParameters
   */
  CBOR_DECODE(ConstructParameters, construct_params) {
    s.list() >> construct_params.signers >> construct_params.threshold
        >> construct_params.unlock_duration;
    return s;
  }

  /**
   * CBOR serialization of ProposeParameters
   */
  CBOR_ENCODE(ProposeParameters, propose_params) {
    return s << (s.list() << propose_params.to << propose_params.value
                          << propose_params.method << propose_params.params);
  }

  /**
   * CBOR deserialization of ProposeParameters
   */
  CBOR_DECODE(ProposeParameters, propose_params) {
    s.list() >> propose_params.to >> propose_params.value
        >> propose_params.method >> propose_params.params;
    return s;
  }

  /**
   * CBOR serialization of TransactionNumberParameters
   */
  CBOR_ENCODE(TransactionNumberParameters, params) {
    return s << (s.list() << params.transaction_number);
  }

  /**
   * CBOR deserialization of TransactionNumberParameters
   */
  CBOR_DECODE(TransactionNumberParameters, params) {
    s.list() >> params.transaction_number;
    return s;
  }

  /**
   * CBOR serialization of AddSignerParameters
   */
  CBOR_ENCODE(AddSignerParameters, params) {
    return s << (s.list() << params.signer << params.increase_threshold);
  }

  /**
   * CBOR deserialization of AddSignerParameters
   */
  CBOR_DECODE(AddSignerParameters, params) {
    s.list() >> params.signer >> params.increase_threshold;
    return s;
  }

  /**
   * CBOR serialization of RemoveSignerParameters
   */
  CBOR_ENCODE(RemoveSignerParameters, params) {
    return s << (s.list() << params.signer << params.decrease_threshold);
  }

  /**
   * CBOR deserialization of RemoveSignerParameters
   */
  CBOR_DECODE(RemoveSignerParameters, params) {
    s.list() >> params.signer >> params.decrease_threshold;
    return s;
  }

  /**
   * CBOR serialization of SwapSignerParameters
   */
  CBOR_ENCODE(SwapSignerParameters, params) {
    return s << (s.list() << params.old_signer << params.new_signer);
  }

  /**
   * CBOR deserialization of SwapSignerParameters
   */
  CBOR_DECODE(SwapSignerParameters, params) {
    s.list() >> params.old_signer >> params.new_signer;
    return s;
  }

  /**
   * CBOR serialization of ChangeThresholdParameters
   */
  CBOR_ENCODE(ChangeThresholdParameters, params) {
    return s << (s.list() << params.new_threshold);
  }

  /**
   * CBOR deserialization of ChangeThresholdParameters
   */
  CBOR_DECODE(ChangeThresholdParameters, params) {
    s.list() >> params.new_threshold;
    return s;
  }

}  // namespace fc::vm::actor::builtin::multisig

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_MULTISIG_ACTOR_HPP
