/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_EVENTS_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_EVENTS_HPP

#include <utility>

#include "miner/storage_fsm/types.hpp"

namespace fc::mining {
  /**
   * SealingEventId is an id event that occurs in a sealing lifecycle
   */
  enum class SealingEventId {
    kSectorStart = 1,
    kSectorStartWithPieces,
    kSectorAddPiece,
    kSectorStartPacking,
    kSectorPacked,
    kSectorPreCommit1,
    kSectorPreCommit2,
    kSectorPreCommitLanded,
    kSectorPreCommitted,
    kSectorSeedReady,
    kSectorCommitted,
    kSectorProving,
    kSectorFinalized,

    kSectorPackingFailed,
    kSectorSealPreCommit1Failed,
    kSectorSealPreCommit2Failed,
    kSectorChainPreCommitFailed,
    kSectorComputeProofFailed,
    kSectorCommitFailed,
    kSectorFinalizeFailed,

    kSectorRetryFinalize,
    kSectorRetrySealPreCommit1,
    kSectorRetrySealPreCommit2,
    kSectorRetryPreCommit,
    kSectorRetryWaitSeed,
    kSectorRetryPreCommitWait,
    kSectorRetryComputeProof,
    kSectorRetryInvalidProof,
    kSectorRetryCommitWait,

    kSectorFaulty,
    kSectorFaultReported,
    kSectorFaultedFinal,

    kSectorRemove,
    kSectorRemoved,
    kSectorRemoveFailed,
  };

  class SealingEvent {
   public:
    virtual ~SealingEvent() = default;

    virtual void apply(const std::shared_ptr<types::SectorInfo> &info) = 0;

   protected:
    SealingEvent(SealingEventId event_id) : event_id_(event_id) {}

   private:
    const SealingEventId event_id_;

    friend bool operator==(const std::shared_ptr<SealingEvent> &lhs,
                           const std::shared_ptr<SealingEvent> &rhs);
  };

  bool operator==(const std::shared_ptr<SealingEvent> &lhs,
                  const std::shared_ptr<SealingEvent> &rhs) {
    return lhs->event_id_ == rhs->event_id_;
  };

  struct SectorStartEvent final : public SealingEvent {
   public:
    SectorStartEvent() : SealingEvent(SealingEventId::kSectorStart){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->sector_number = sector_id;
      info->sector_type = seal_proof_type;
    }

    SectorNumber sector_id;
    RegisteredProof seal_proof_type;
  };

  struct SectorStartWithPiecesEvent final : public SealingEvent {
   public:
    SectorStartWithPiecesEvent()
        : SealingEvent(SealingEventId::kSectorStartWithPieces){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->sector_number = sector_id;
      info->sector_type = seal_proof_type;
      info->pieces = pieces;
    }

    SectorNumber sector_id;
    RegisteredProof seal_proof_type;
    std::vector<types::Piece> pieces;
  };

  struct SectorAddPieceEvent final : public SealingEvent {
   public:
    SectorAddPieceEvent() : SealingEvent(SealingEventId::kSectorAddPiece){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->pieces.push_back(piece);
    }

    types::Piece piece;
  };

  struct SectorStartPackingEvent final : public SealingEvent {
   public:
    SectorStartPackingEvent()
        : SealingEvent(SealingEventId::kSectorStartPacking){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorPackedEvent final : public SealingEvent {
   public:
    SectorPackedEvent() : SealingEvent(SealingEventId::kSectorPacked){};

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

  struct SectorPreCommit1Event final : public SealingEvent {
   public:
    SectorPreCommit1Event() : SealingEvent(SealingEventId::kSectorPreCommit1){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->precommit1_output = precommit1_output;
      info->ticket = ticket;
      info->ticket_epoch = epoch;
      info->precommit2_fails = 0;
    }

    types::PreCommit1Output precommit1_output;
    types::SealRandomness ticket;
    types::ChainEpoch epoch;
  };

  struct SectorPreCommit2Event final : public SealingEvent {
   public:
    SectorPreCommit2Event() : SealingEvent(SealingEventId::kSectorPreCommit2){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->comm_d = unsealed;
      info->comm_r = sealed;
    }

    CID unsealed;
    CID sealed;
  };

  struct SectorPreCommitLandedEvent final : public SealingEvent {
   public:
    SectorPreCommitLandedEvent()
        : SealingEvent(SealingEventId::kSectorPreCommitLanded){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->precommit_tipset = tipset_key;
    }

    types::TipsetKey tipset_key;
  };

  struct SectorPreCommittedEvent final : public SealingEvent {
   public:
    SectorPreCommittedEvent()
        : SealingEvent(SealingEventId::kSectorPreCommitted){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->precommit_message = precommit_message;
      info->precommit_deposit = precommit_deposit;
      info->precommit_info = precommit_info;
    }

    CID precommit_message;
    primitives::TokenAmount precommit_deposit;
    types::SectorPreCommitInfo precommit_info;
  };

  struct SectorSeedReadyEvent final : public SealingEvent {
   public:
    SectorSeedReadyEvent() : SealingEvent(SealingEventId::kSectorSeedReady){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->seed = seed;
      info->seed_epoch = epoch;
    }

    types::InteractiveRandomness seed;
    ChainEpoch epoch;
  };

  struct SectorCommittedEvent final : public SealingEvent {
   public:
    SectorCommittedEvent() : SealingEvent(SealingEventId::kSectorCommitted){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->proof = proof;
      info->message = message;
    }

    CID message;
    proofs::Proof proof;
  };

  struct SectorProvingEvent final : public SealingEvent {
   public:
    SectorProvingEvent() : SealingEvent(SealingEventId::kSectorProving){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorFinalizedEvent final : public SealingEvent {
   public:
    SectorFinalizedEvent() : SealingEvent(SealingEventId::kSectorFinalized){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  // ERROR

  struct SectorPackingFailedEvent final : public SealingEvent {
   public:
    SectorPackingFailedEvent()
        : SealingEvent(SealingEventId::kSectorPackingFailed){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorSealPreCommit1FailedEvent final : public SealingEvent {
   public:
    SectorSealPreCommit1FailedEvent()
        : SealingEvent(SealingEventId::kSectorSealPreCommit1Failed){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->invalid_proofs = 0;
      info->precommit2_fails = 0;
    }
  };

  struct SectorSealPreCommit2FailedEvent final : public SealingEvent {
   public:
    SectorSealPreCommit2FailedEvent()
        : SealingEvent(SealingEventId::kSectorSealPreCommit2Failed){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->invalid_proofs = 0;
      info->precommit2_fails++;
    }
  };

  struct SectorChainPreCommitFailedEvent final : public SealingEvent {
   public:
    SectorChainPreCommitFailedEvent()
        : SealingEvent(SealingEventId::kSectorChainPreCommitFailed){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorComputeProofFailedEvent final : public SealingEvent {
   public:
    SectorComputeProofFailedEvent()
        : SealingEvent(SealingEventId::kSectorComputeProofFailed){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorCommitFailedEvent final : public SealingEvent {
   public:
    SectorCommitFailedEvent()
        : SealingEvent(SealingEventId::kSectorCommitFailed){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorFinalizeFailedEvent final : public SealingEvent {
   public:
    SectorFinalizeFailedEvent()
        : SealingEvent(SealingEventId::kSectorFinalizeFailed){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  // RETRY

  struct SectorRetryFinalizeEvent final : public SealingEvent {
   public:
    SectorRetryFinalizeEvent()
        : SealingEvent(SealingEventId::kSectorRetryFinalize){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorRetrySealPreCommit1Event final : public SealingEvent {
   public:
    SectorRetrySealPreCommit1Event()
        : SealingEvent(SealingEventId::kSectorRetrySealPreCommit1){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorRetrySealPreCommit2Event final : public SealingEvent {
   public:
    SectorRetrySealPreCommit2Event()
        : SealingEvent(SealingEventId::kSectorRetrySealPreCommit2){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorRetryPreCommitEvent final : public SealingEvent {
   public:
    SectorRetryPreCommitEvent()
        : SealingEvent(SealingEventId::kSectorRetryPreCommit){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorRetryWaitSeedEvent final : public SealingEvent {
   public:
    SectorRetryWaitSeedEvent()
        : SealingEvent(SealingEventId::kSectorRetryWaitSeed){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorRetryPreCommitWaitEvent final : public SealingEvent {
   public:
    SectorRetryPreCommitWaitEvent()
        : SealingEvent(SealingEventId::kSectorRetryPreCommitWait){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorRetryComputeProofEvent final : public SealingEvent {
   public:
    SectorRetryComputeProofEvent()
        : SealingEvent(SealingEventId::kSectorRetryComputeProof){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->invalid_proofs++;
    }
  };

  struct SectorRetryInvalidProofEvent final : public SealingEvent {
   public:
    SectorRetryInvalidProofEvent()
        : SealingEvent(SealingEventId::kSectorRetryInvalidProof){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->invalid_proofs++;
    }
  };

  struct SectorRetryCommitWaitEvent final : public SealingEvent {
   public:
    SectorRetryCommitWaitEvent()
        : SealingEvent(SealingEventId::kSectorRetryCommitWait){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  // FAULTS

  struct SectorFaultyEvent final : public SealingEvent {
   public:
    SectorFaultyEvent() : SealingEvent(SealingEventId::kSectorFaulty){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorFaultReportedEvent final : public SealingEvent {
   public:
    SectorFaultReportedEvent()
        : SealingEvent(SealingEventId::kSectorFaultReported){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->fault_report_message = report_message;
    }

    CID report_message;
  };

  struct SectorFaultedFinalEvent final : public SealingEvent {
   public:
    SectorFaultedFinalEvent()
        : SealingEvent(SealingEventId::kSectorFaultedFinal){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  // EXTERNAL EVENTS

  struct SectorRemoveEvent final : public SealingEvent {
   public:
    SectorRemoveEvent() : SealingEvent(SealingEventId::kSectorRemove){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorRemovedEvent final : public SealingEvent {
   public:
    SectorRemovedEvent() : SealingEvent(SealingEventId::kSectorRemoved){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };

  struct SectorRemoveFailedEvent final : public SealingEvent {
   public:
    SectorRemoveFailedEvent()
        : SealingEvent(SealingEventId::kSectorRemoveFailed){};

    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_EVENTS_HPP
