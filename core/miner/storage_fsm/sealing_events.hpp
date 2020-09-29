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
  enum class SealingEvent {
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

    kSectorForce,
  };

  class SealingEventContext {
   public:
    virtual ~SealingEventContext() = default;

    virtual void apply(const std::shared_ptr<types::SectorInfo> &info) = 0;
  };

  struct SectorStartEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->sector_number = sector_id;
      info->sector_type = seal_proof_type;
    }

    SectorNumber sector_id;
    RegisteredProof seal_proof_type;
  };

  struct SectorStartWithPiecesEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->sector_number = sector_id;
      info->sector_type = seal_proof_type;
      info->pieces = pieces;
    }

    SectorNumber sector_id;
    RegisteredProof seal_proof_type;
    std::vector<types::Piece> pieces;
  };

  struct SectorAddPieceEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->pieces.push_back(piece);
    }

    types::Piece piece;
  };

  struct SectorPackedEvent final : public SealingEventContext {
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

  struct SectorPreCommit1Event final : public SealingEventContext {
   public:
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

  struct SectorPreCommit2Event final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->comm_d = unsealed;
      info->comm_r = sealed;
    }

    CID unsealed;
    CID sealed;
  };

  struct SectorPreCommitLandedEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->precommit_tipset = tipset_key;
    }

    types::TipsetKey tipset_key;
  };

  struct SectorPreCommittedEvent final : public SealingEventContext {
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

  struct SectorSeedReadyEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->seed = seed;
      info->seed_epoch = epoch;
    }

    types::InteractiveRandomness seed;
    ChainEpoch epoch;
  };

  struct SectorCommittedEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->proof = proof;
      info->message = message;
    }

    CID message;
    proofs::Proof proof;
  };

  // ERROR

  struct SectorSealPreCommit1FailedEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->invalid_proofs = 0;
      info->precommit2_fails = 0;
    }
  };

  struct SectorSealPreCommit2FailedEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->invalid_proofs = 0;
      info->precommit2_fails++;
    }
  };

  // RETRY

  struct SectorRetryComputeProofEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->invalid_proofs++;
    }
  };

  struct SectorRetryInvalidProofEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->invalid_proofs++;
    }
  };

  // FAULTS

  struct SectorFaultReportedEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {
      info->fault_report_message = report_message;
    }

    CID report_message;
  };

  // EXTERNAL EVENTS

  struct SectorForceEvent final : public SealingEventContext {
   public:
    void apply(const std::shared_ptr<types::SectorInfo> &info) override {}

    SealingState state;
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_EVENTS_HPP
