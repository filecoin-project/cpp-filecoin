/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <utility>

#include "miner/storage_fsm/types.hpp"

namespace fc::mining {
  using primitives::sector::RegisteredSealProof;

  /**
   * SealingEventId is an id event that occurs in a sealing lifecycle
   */
  enum class SealingEvent {
    kSectorStart = 1,
    kSectorStartWithPieces,
    kSectorAddPieces,
    kSectorRetryWaitDeals,
    kSectorStartPacking,
    kSectorPacked,
    kSectorPreCommit1,
    kSectorPreCommit2,
    kSectorPreCommitLanded,
    kSectorPreCommitted,
    kSectorBatchSend,
    kSectorPreCommittedBatch,
    kSectorSeedReady,
    kSectorComputeProof,
    kSectorCommitted,
    kSectorProving,
    kSectorFinalized,
    kSectorUpdateDealIDs,

    kSectorAddPieceFailed,
    kSectorSealPreCommit1Failed,
    kSectorSealPreCommit2Failed,
    kSectorChainPreCommitFailed,
    kSectorComputeProofFailed,
    kSectorCommitFailed,
    kSectorFinalizeFailed,
    kSectorDealsExpired,
    kSectorInvalidDealIDs,

    kSectorRetryFinalize,
    kSectorRetrySealPreCommit1,
    kSectorRetrySealPreCommit2,
    kSectorRetryPreCommit,
    kSectorRetryWaitSeed,
    kSectorRetryPreCommitWait,
    kSectorRetryComputeProof,
    kSectorRetryInvalidProof,
    kSectorRetryCommitting,
    kSectorRetryCommitWait,

    kSectorFaulty,
    kSectorFaultReported,
    kSectorFaultedFinal,

    kSectorRemove,
    kSectorRemoved,
    kSectorRemoveFailed,

    kSectorForce,
    kUpdateDealIds,

    // Snap Deals
    kSectorStartCCUpdate,
    kSectorReplicaUpdate,
    kSectorProveReplicaUpdate,
    kSectorReplicaUpdateSubmitted,
    kSectorReplicaUpdateLanded,
    kSectorRetryReplicaUpdate,
    kSectorRetryProveReplicaUpdate,
    kSectorKeyReleased,

    // Snap Deals failure and recovery
    kSectorUpdateReplicaFailed,
    kSectorProveReplicaUpdateFailed,
    kSectorAbortUpgrade,
    kSectorUpdateActive,
    kSectorReleaseKeyFailed,
    kSectorRevertUpgradeToProving,
    kSectorRetrySubmitReplicaUpdateWait,
    kSectorRetrySubmitReplicaUpdate,
    kSectorSubmitReplicaUpdateFailed,
  };

  class SealingEventContext {
   public:
    virtual ~SealingEventContext() = default;

    virtual void apply(const std::shared_ptr<types::SectorInfo> &info) = 0;
  };

  struct SectorStartContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->sector_number = sector_id;
      info->sector_type = seal_proof_type;
    }

    SectorNumber sector_id{};
    RegisteredSealProof seal_proof_type{RegisteredSealProof::kUndefined};
  };

  struct SectorStartWithPiecesContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->sector_number = sector_id;
      info->sector_type = seal_proof_type;
      info->pieces = pieces;
    }

    SectorNumber sector_id{};
    RegisteredSealProof seal_proof_type{RegisteredSealProof::kUndefined};
    std::vector<types::Piece> pieces;
  };

  struct SectorAddPiecesContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->pieces.insert(info->pieces.end(), pieces.begin(), pieces.end());
    }

    std::vector<types::Piece> pieces;
  };

  struct SectorPackedContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      for (const auto &piece : filler_pieces) {
        info->pieces.push_back({
            .piece = piece,
            .deal_info = boost::none,
        });
      }
    }

    std::vector<types::PieceInfo> filler_pieces;
  };

  struct SectorPreCommit1Context final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->precommit1_output = precommit1_output;
      info->ticket = ticket;
      info->ticket_epoch = epoch;
      info->precommit2_fails = 0;
    }

    types::PreCommit1Output precommit1_output;
    types::SealRandomness ticket;
    types::ChainEpoch epoch{};
  };

  struct SectorPreCommit2Context final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->comm_d = unsealed;
      info->comm_r = sealed;
    }

    CID unsealed;
    CID sealed;
  };

  struct SectorPreCommitLandedContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->precommit_tipset = tipset_key.cids();
    }

    types::TipsetKey tipset_key;
  };

  struct SectorPreCommittedContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->precommit_message = precommit_message;
      info->precommit_deposit = precommit_deposit;
      info->precommit_info = precommit_info;
    }

    CID precommit_message;
    primitives::TokenAmount precommit_deposit;
    types::SectorPreCommitInfo precommit_info;
  };

  struct SectorPreCommittedBatchContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->precommit_message = precommit_message;
    }

    CID precommit_message;
  };

  struct SectorSeedReadyContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->seed = seed;
      info->seed_epoch = epoch;
    }

    types::InteractiveRandomness seed;
    ChainEpoch epoch{};
  };

  struct SectorComputeProofContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->proof = proof;
    }

    proofs::Proof proof;
  };

  struct SectorCommittedContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->message = message;
    }

    CID message;
  };

  // FAULTS

  struct SectorFaultReportedContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->fault_report_message = report_message;
    }

    CID report_message;
  };

  struct SectorInvalidDealIDContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->return_state = return_state;
    }

    SealingState return_state{SealingState::kStateUnknown};
  };

  // SNAP DEAL

  struct SectorStartCCUpdateContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->update = true;
      // Clear filler piece but remember in case of abort
      info->update_pieces = info->pieces;
      info->pieces.clear();
    }
  };

  struct SectorReplicaUpdateContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->update_sealed = update_sealed;
      info->update_unsealed = update_unsealed;
    }

    CID update_sealed;
    CID update_unsealed;
  };

  struct SectorProveReplicaUpdateContext final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->update_proof = proof;
    }

    sector_storage::ReplicaUpdateProof proof;
  };

  struct SectorReplicaUpdateSubmittedContext final
      : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->update_message = message;
    }

    CID message;
  };

  struct SectorRevertUpgradeToProvingContext final
      : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->update = false;
      info->update_sealed = boost::none;
      info->update_unsealed = boost::none;
      info->update_proof = boost::none;
      info->update_message = boost::none;
      info->pieces = std::move(info->update_pieces);
    }
  };

  // EXTERNAL EVENTS

  struct SectorForceContext : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}

    SealingState state{SealingState::kStateUnknown};
  };

  struct SectorUpdateDealIds final : public SectorForceContext {
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      state = info->return_state;
      info->return_state = SealingState::kStateUnknown;
      for (auto &[piece_index, id] : updates) {
        info->pieces[piece_index].deal_info->deal_id = id;
      }
    }

    std::unordered_map<uint64_t, primitives::DealId> updates;
  };
}  // namespace fc::mining
