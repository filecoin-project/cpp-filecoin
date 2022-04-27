/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

#include "vm/actor/builtin/types/multisig/transaction.hpp"

namespace fc::vm::actor::builtin::multisig {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using types::multisig::ProposalHashData;
  using types::multisig::Transaction;
  using types::multisig::TransactionId;

  // These methods must be actual with the last version of actors

  enum class MultisigActor : MethodNumber {
    kConstruct = 1,
    kPropose,
    kApprove,
    kCancel,
    kAddSigner,
    kRemoveSigner,
    kSwapSigner,
    kChangeThreshold,
    kLockBalance,
  }

  struct Construct : ActorMethodBase<MultisigActor::kConstruct> {
    struct Params {
      std::vector<Address> signers;
      size_t threshold{};
      EpochDuration unlock_duration{};
      ChainEpoch start_epoch{};

      inline bool operator==(const Params &other) const {
        return signers == other.sighers && threshold == other.threshold
               && unlock_duration == other.unlock_duration
               && start_epoch == other.start_epoch;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(
      Construct::Params, signers, threshold, unlock_duration, start_epoch)

  struct Propose : ActorMethodBase<MultisigActor::kPropose> {
    struct Params {
      Address to;
      TokenAmount value;
      MethodNumber method{};
      MethodParams params;

      inline bool operator==(const Params &other) const {
        return to == other.to && value == other.value && method == other.method
               && params == other.method;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };

    struct Result {
      TransactionId tx_id{};
      bool applied{false};
      VMExitCode code{};
      Bytes return_value;

      inline bool operator==(const Result &other) const {
        return tx_id == other.tx_id && applied == other.applied
               && code == other.code && return_value == other.return_value;
      }

      inline bool operator!=(const Result &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(Propose::Params, to, value, method, params)
  CBOR_TUPLE(Propose::Result, tx_id, applied, code, return_value)

  struct Approve : ActorMethodBase<MultisigActor::kApprove> {
    struct Params {
      TransactionId tx_id{};
      Bytes proposal_hash;

      inline bool operator==(const Params &other) const {
        return tx_id == other.tx_id && proposal_hash == other.proposal_hash;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };

    struct Result {
      bool applied{false};
      VMExitCode code{};
      Bytes return_value;

      inline bool operator==(const Result &other) const {
        return applied == other.applied && code == other.code
               && return_value == other.return_value;
      }

      inline bool operator!=(const Result &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(Approve::Params, tx_id, proposal_hash)
  CBOR_TUPLE(Approve::Result, applied, code, return_value)

  struct Cancel : ActorMethodBase<MultisigActor::kCancel> {
    struct Params {
      TransactionId tx_id{};
      Bytes proposal_hash;

      inline bool operator==(const Params &other) const {
        return tx_id == other.tx_id && proposal_hash == other.proposal_hash;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(Cancel::Params, tx_id, proposal_hash)

  struct AddSigner : ActorMethodBase<MultisigActor::kAddSigner> {
    struct Params {
      Address signer;
      bool increase_threshold{false};

      inline bool operator==(const Params &other) const {
        return signer == other.signer
               && increase_threshold == other.increase_threshold;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(AddSigner::Params, signer, increase_threshold)

  struct RemoveSigner : ActorMethodBase<MultisigActor::kRemoveSigner> {
    struct Params {
      Address signer;
      bool decrease_threshold{false};

      inline bool operator==(const Params &other) const {
        return signer == other.signer
               && decrease_threshold == other.decrease_threshold;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(RemoveSigner::Params, signer, decrease_threshold)

  struct SwapSigner : ActorMethodBase<MultisigActor::kSwapSigner> {
    struct Params {
      Address from;
      Address to;

      inline bool operator==(const Params &other) const {
        return from == other.from && to == other.to;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(SwapSigner::Params, from, to)

  struct ChangeThreshold : ActorMethodBase<MultisigActor::kChangeThreshold> {
    struct Params {
      size_t new_threshold{};

      inline bool operator==(const Params &other) const {
        return new_threshold == other.new_threshold;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(ChangeThreshold::Params, new_threshold)

  struct LockBalance : ActorMethodBase<MultisigActor::kLockBalance> {
    struct Params {
      ChainEpoch start_epoch{};
      EpochDuration unlock_duration{};
      TokenAmount amount;

      inline bool operator==(const Params &other) const {
        return start_epoch == other.start_epoch
               && unlock_duration == other.unlock_duration
               && amount == other.amount;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(LockBalance::Params, start_epoch, unlock_duration, amount)

}  // namespace fc::vm::actor::builtin::multisig
