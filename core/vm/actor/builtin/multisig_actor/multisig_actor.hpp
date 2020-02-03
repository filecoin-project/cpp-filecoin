/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_MULTISIG_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_MULTISIG_ACTOR_HPP

#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch.hpp"
#include "primitives/cid/cid.hpp"
#include "vm/actor/actor.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin {

  using primitives::EpochDuration;
  using primitives::UBigInt;
  using primitives::address::Address;
  using runtime::InvocationOutput;
  using runtime::Runtime;
  using TransactionNumber = size_t;
  using ChainEpoch = BigInt;

  struct MultiSignatureTransaction {
    TransactionNumber transaction_number;
    Address to;
    BigInt value;
    MethodNumber method;
    MethodParams params;
    // This address at index 0 is the transaction proposer, order of this slice
    // must be preserved.
    std::vector<Address> approved;
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const MultiSignatureTransaction &transaction) {
    return s << (s.list() << transaction.transaction_number << transaction.to
                          << transaction.value << transaction.method
                          << transaction.params << transaction.approved);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, MultiSignatureTransaction &transaction) {
    s.list() >> transaction.transaction_number >> transaction.to
        >> transaction.value >> transaction.method >> transaction.params
        >> transaction.approved;
    return s;
  }

  /**
   * State of Mutisig Actor instance
   */
  struct MultiSignatureActorState {
    std::vector<Address> signers;
    size_t threshold;
    TransactionNumber next_transaction_id;

    // Linear lock
    BigInt initial_balance;
    ChainEpoch start_epoch;
    EpochDuration unlock_duration;

    std::vector<MultiSignatureTransaction> pending_transactions;

    bool isSigner(const Address &address) const;
    outcome::result<void> approveTransaction(const Actor &actor,
                                             Runtime &runtime,
                                             const TransactionNumber &tx_numer);
    BigInt getAmountLocked(const ChainEpoch &current_epoch) const;
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const MultiSignatureActorState &state) {
    return s << (s.list() << state.signers << state.threshold
                          << state.next_transaction_id << state.initial_balance
                          << state.start_epoch << state.unlock_duration
                          << state.pending_transactions);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, MultiSignatureActorState &state) {
    s.list() >> state.signers >> state.threshold >> state.next_transaction_id
        >> state.initial_balance >> state.start_epoch >> state.unlock_duration
        >> state.pending_transactions;
    return s;
  }

  /**
   * Construct method parameters
   */
  struct ConstructParameteres {
    std::vector<Address> signers;
    size_t threshold;
    EpochDuration unlock_duration;
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const ConstructParameteres &construct_params) {
    return s << (s.list() << construct_params.signers
                          << construct_params.threshold
                          << construct_params.unlock_duration);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, ConstructParameteres &construct_params) {
    s.list() >> construct_params.signers >> construct_params.threshold
        >> construct_params.unlock_duration;
    return s;
  }

  struct ProposeParameters {
    Address to;
    BigInt value;
    MethodNumber method;
    MethodParams params;
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const ProposeParameters &propose_params) {
    return s << (s.list() << propose_params.to << propose_params.value
                          << propose_params.method << propose_params.params);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, ProposeParameters &propose_params) {
    s.list() >> propose_params.to >> propose_params.value
        >> propose_params.method >> propose_params.params;
    return s;
  }

  struct TransactionNumberParameters {
    TransactionNumber transaction_number;
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const TransactionNumberParameters &params) {
    return s << (s.list() << params.transaction_number);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, TransactionNumberParameters &params) {
    s.list() >> params.transaction_number;
    return s;
  }

  struct AddSignerParameters {
    Address signer;
    bool increase_threshold{false};
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const AddSignerParameters &params) {
    return s << (s.list() << params.signer << params.increase_threshold);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, AddSignerParameters &params) {
    s.list() >> params.signer >> params.increase_threshold;
    return s;
  }

  struct RemoveSignerParameters {
    Address signer;
    bool decrease_threshold{false};
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const RemoveSignerParameters &params) {
    return s << (s.list() << params.signer << params.decrease_threshold);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, RemoveSignerParameters &params) {
    s.list() >> params.signer >> params.decrease_threshold;
    return s;
  }

  struct SwapSignerParameters {
    Address old_signer;
    Address new_signer;
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const SwapSignerParameters &params) {
    return s << (s.list() << params.old_signer << params.new_signer);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, SwapSignerParameters &params) {
    s.list() >> params.old_signer >> params.new_signer;
    return s;
  }

  struct ChangeTresholdParameters {
    size_t new_threshold;
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const ChangeTresholdParameters &params) {
    return s << (s.list() << params.new_threshold);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, ChangeTresholdParameters &params) {
    s.list() >> params.new_threshold;
    return s;
  }

  //  struct ValidateSignerParameters {
  //    Address address;
  //  };
  //
  //  template <class Stream,
  //            typename = std::enable_if_t<
  //                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  //  Stream &operator<<(Stream &&s, const ValidateSignerParameters &params) {
  //    return s << (s.list() << params.address);
  //  }
  //
  //  template <class Stream,
  //            typename = std::enable_if_t<
  //                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  //  Stream &operator>>(Stream &&s, ValidateSignerParameters &params) {
  //    s.list() >> params.address;
  //    return s;
  //  }

  /**
   * Multisignature actor
   */
  class MultiSigActor {
   public:
    static constexpr VMExitCode WRONG_CALLER{1};
    static constexpr VMExitCode NOT_SIGNER{2};
    static constexpr VMExitCode ALREADY_SIGNED{3};
    static constexpr VMExitCode TRANSACTION_NOT_FOUND{4};
    static constexpr VMExitCode FUNDS_LOCKED{5};

    static outcome::result<InvocationOutput> construct(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> propose(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> approve(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> cancel(const Actor &actor,
                                                    Runtime &runtime,
                                                    const MethodParams &params);

    static outcome::result<InvocationOutput> add_signer(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> remove_signer(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> swap_signer(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> change_threshold(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    //    static outcome::result<InvocationOutput> validate_signer(
    //        const Actor &actor, Runtime &runtime, const MethodParams
    //        &params);
  };

}  // namespace fc::vm::actor::builtin

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_MULTISIG_ACTOR_HPP
