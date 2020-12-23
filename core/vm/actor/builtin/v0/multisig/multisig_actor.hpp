/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_MULTISIG_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_MULTISIG_ACTOR_HPP

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::multisig {

  struct Construct : ActorMethodBase<1> {
    struct Params {
      std::vector<Address> signers;
      size_t threshold{};
      EpochDuration unlock_duration{};
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
    struct Result {
      TransactionId tx_id{};
      bool applied{false};
      VMExitCode code;
      Buffer return_value;

      inline bool operator==(const Result &rhs) const {
        return tx_id == rhs.tx_id
        && applied == rhs.applied
        && code == rhs.code
        && return_value == rhs.return_value;
      }
    };

    ACTOR_METHOD_DECL();
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
        return applied == rhs.applied
        && code == rhs.code
        && return_value == rhs.return_value;
      }
    };

    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Approve::Params, tx_id, proposal_hash)
  CBOR_TUPLE(Approve::Result, applied, code, return_value)

  struct Cancel : ActorMethodBase<4> {
    struct Params {
      TransactionId tx_id{};
      Buffer proposal_hash;
    };

    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Cancel::Params, tx_id, proposal_hash)

  struct AddSigner : ActorMethodBase<5> {
    struct Params {
      Address signer;
      bool increase_threshold{false};
    };

    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(AddSigner::Params, signer, increase_threshold)

  struct RemoveSigner : ActorMethodBase<6> {
    struct Params {
      Address signer;
      bool decrease_threshold{false};
    };

    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(RemoveSigner::Params, signer, decrease_threshold)

  struct SwapSigner : ActorMethodBase<7> {
    struct Params {
      Address from;
      Address to;
    };

    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(SwapSigner::Params, from, to)

  struct ChangeThreshold : ActorMethodBase<8> {
    struct Params {
      size_t new_threshold{};
    };

    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ChangeThreshold::Params, new_threshold)

  struct LockBalance : ActorMethodBase<9> {
    struct Params {
      ChainEpoch start_epoch{};
      EpochDuration unlock_duration{};
      TokenAmount amount{};
    };

    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(LockBalance::Params, start_epoch, unlock_duration, amount)

  /** Exported Multisig Actor methods to invoker */
  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v0::multisig

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_V0_MULTISIG_ACTOR_HPP
