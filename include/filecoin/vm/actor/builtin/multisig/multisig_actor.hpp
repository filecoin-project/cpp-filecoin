/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_MULTISIG_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_MULTISIG_ACTOR_HPP

#include "filecoin/codec/cbor/streams_annotation.hpp"
#include "filecoin/primitives/address/address.hpp"
#include "filecoin/primitives/address/address_codec.hpp"
#include "filecoin/primitives/big_int.hpp"
#include "filecoin/primitives/chain_epoch/chain_epoch.hpp"
#include "filecoin/primitives/cid/cid.hpp"
#include "filecoin/vm/actor/actor.hpp"
#include "filecoin/vm/actor/actor_method.hpp"
#include "filecoin/vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::multisig {

  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::address::Address;
  using runtime::InvocationOutput;
  using runtime::Runtime;
  using TransactionNumber = size_t;

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
  CBOR_TUPLE(MultiSignatureTransaction,
             transaction_number,
             to,
             value,
             method,
             params,
             approved)

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
        Runtime &runtime, const TransactionNumber &tx_number);

    /**
     * Get amount locked for current epoch
     * @param current_epoch - current block number
     * @return - return amount locked
     */
    BigInt getAmountLocked(const ChainEpoch &current_epoch) const;
  };
  CBOR_TUPLE(MultiSignatureActorState,
             signers,
             threshold,
             next_transaction_id,
             initial_balance,
             start_epoch,
             unlock_duration,
             pending_transactions)

  struct Construct : ActorMethodBase<1> {
    struct Params {
      std::vector<Address> signers;
      size_t threshold;
      EpochDuration unlock_duration;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Construct::Params, signers, threshold, unlock_duration)

  struct Propose : ActorMethodBase<2> {
    struct Params {
      Address to;
      BigInt value;
      MethodNumber method{};
      MethodParams params;
    };
    using Result = TransactionNumber;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Propose::Params, to, value, method, params)

  struct Approve : ActorMethodBase<3> {
    struct Params {
      TransactionNumber transaction_number;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Approve::Params, transaction_number)

  struct Cancel : ActorMethodBase<4> {
    struct Params {
      TransactionNumber transaction_number;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Cancel::Params, transaction_number)

  struct AddSigner : ActorMethodBase<6> {
    struct Params {
      Address signer;
      bool increase_threshold{false};
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(AddSigner::Params, signer, increase_threshold)

  struct RemoveSigner : ActorMethodBase<7> {
    struct Params {
      Address signer;
      bool decrease_threshold{false};
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(RemoveSigner::Params, signer, decrease_threshold)

  struct SwapSigner : ActorMethodBase<8> {
    struct Params {
      Address old_signer;
      Address new_signer;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(SwapSigner::Params, old_signer, new_signer)

  struct ChangeThreshold : ActorMethodBase<9> {
    struct Params {
      size_t new_threshold;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ChangeThreshold::Params, new_threshold)

  /** Exported Multisig Actor methods to invoker */
  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::multisig

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_MULTISIG_ACTOR_HPP
