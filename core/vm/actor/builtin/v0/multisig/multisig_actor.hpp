/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/multisig/multisig_actor_state.hpp"
#include "vm/actor/builtin/v0/multisig/multisig_utils.hpp"

namespace fc::vm::actor::builtin::v0::multisig {
  using utils::multisig::MultisigUtils;

  struct Construct : ActorMethodBase<1> {
    struct Params {
      std::vector<Address> signers;
      size_t threshold{};
      EpochDuration unlock_duration{};
    };

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<std::vector<Address>> getResolvedSigners(
        const Runtime &runtime, const std::vector<Address> &signers);
    static outcome::result<void> checkParams(
        const std::vector<Address> &signers,
        size_t threshold,
        const EpochDuration &unlock_duration);
    static State createState(Runtime &runtime,
                             size_t threshold,
                             const std::vector<Address> &signers);
    static void setLocked(const Runtime &runtime,
                          const EpochDuration &unlock_duration,
                          State &state);
  };
  CBOR_TUPLE(Construct::Params, signers, threshold, unlock_duration)

  struct Propose : ActorMethodBase<2> {
    struct Params {
      Address to;
      BigInt value;
      MethodNumber method{};
      MethodParams params;
    };
    struct Result {
      TransactionId tx_id{};
      bool applied{false};
      VMExitCode code;
      Buffer return_value;

      inline bool operator==(const Result &rhs) const {
        return tx_id == rhs.tx_id && applied == rhs.applied && code == rhs.code
               && return_value == rhs.return_value;
      }
    };

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<std::tuple<TransactionId, Transaction>>
    createTransaction(const Params &params, State &state);
  };
  CBOR_TUPLE(Propose::Params, to, value, method, params)
  CBOR_TUPLE(Propose::Result, tx_id, applied, code, return_value)

  struct Approve : ActorMethodBase<3> {
    struct Params {
      TransactionId tx_id{};
      Buffer proposal_hash;
    };
    struct Result {
      bool applied{false};
      VMExitCode code;
      Buffer return_value;

      inline bool operator==(const Result &rhs) const {
        return applied == rhs.applied && code == rhs.code
               && return_value == rhs.return_value;
      }
    };

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<Result> approveTransaction(
        Runtime &runtime,
        const Params &params,
        State &state,
        const MultisigUtils &utils);
  };
  CBOR_TUPLE(Approve::Params, tx_id, proposal_hash)
  CBOR_TUPLE(Approve::Result, applied, code, return_value)

  struct Cancel : ActorMethodBase<4> {
    struct Params {
      TransactionId tx_id{};
      Buffer proposal_hash;
    };

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<void> checkTransaction(Runtime &runtime,
                                                  const Params &params,
                                                  const State &state);
    static outcome::result<void> removeTransaction(const Params &params,
                                                   State &state);
  };
  CBOR_TUPLE(Cancel::Params, tx_id, proposal_hash)

  struct AddSigner : ActorMethodBase<5> {
    struct Params {
      Address signer;
      bool increase_threshold{false};
    };

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<void> addSigner(const Params &params,
                                           State &state,
                                           const Address &signer);
  };
  CBOR_TUPLE(AddSigner::Params, signer, increase_threshold)

  struct RemoveSigner : ActorMethodBase<6> {
    struct Params {
      Address signer;
      bool decrease_threshold{false};
    };

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<void> checkState(const Params &params,
                                            const State &state,
                                            const Address &signer);
    static void removeSigner(const Params &params,
                             State &state,
                             const Address &signer);
  };
  CBOR_TUPLE(RemoveSigner::Params, signer, decrease_threshold)

  struct SwapSigner : ActorMethodBase<7> {
    struct Params {
      Address from;
      Address to;
    };

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<void> swapSigner(State &state,
                                            const Address &from,
                                            const Address &to);
  };
  CBOR_TUPLE(SwapSigner::Params, from, to)

  struct ChangeThreshold : ActorMethodBase<8> {
    struct Params {
      size_t new_threshold{};
    };

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<void> changeThreshold(const Params &params,
                                                 State &state);
  };
  CBOR_TUPLE(ChangeThreshold::Params, new_threshold)

  struct LockBalance : ActorMethodBase<9> {
    struct Params {
      ChainEpoch start_epoch{};
      EpochDuration unlock_duration{};
      TokenAmount amount{};
    };

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<void> checkNetwork(const Runtime &runtime);
    static outcome::result<void> lockBalance(const Params &params,
                                             State &state);
  };
  CBOR_TUPLE(LockBalance::Params, start_epoch, unlock_duration, amount)

  /** Exported Multisig Actor methods to invoker */
  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v0::multisig
