/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_COMMON_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_COMMON_HPP

#include "miner/storage_fsm/sealing_states.hpp"
#include "miner/storage_fsm/types.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "primitives/types.hpp"
#include "sector_storage/manager.hpp"
#include "vm/actor/builtin/v0/miner/types.hpp"

namespace fc::mining::types {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::SectorNumber;
  using primitives::piece::PaddedPieceSize;
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::tipset::TipsetKey;
  using proofs::SealRandomness;
  using sector_storage::InteractiveRandomness;
  using sector_storage::PreCommit1Output;
  using vm::actor::builtin::v0::miner::SectorPreCommitInfo;

  constexpr uint64_t kDealSectorPriority = 1024;

  /**
   * DealSchedule communicates the time interval of a storage deal. The deal
   * must appear in a sealed (proven) sector no later than StartEpoch, otherwise
   * it is invalid.
   */
  struct DealSchedule {
    ChainEpoch start_epoch;
    ChainEpoch end_epoch;
  };

  /** DealInfo is a tuple of deal identity and its schedule */
  struct DealInfo {
    DealId deal_id;
    DealSchedule deal_schedule;
  };

  struct Piece {
    PieceInfo piece;
    boost::optional<DealInfo> deal_info;
  };

  struct SectorInfo {
    SealingState state;

    SectorNumber sector_number;
    RegisteredProof sector_type;
    std::vector<Piece> pieces;

    SealRandomness ticket;
    ChainEpoch ticket_epoch;
    PreCommit1Output precommit1_output;
    uint64_t precommit2_fails;

    boost::optional<CID> comm_d;
    boost::optional<CID> comm_r;

    boost::optional<CID> precommit_message;
    primitives::TokenAmount precommit_deposit;
    boost::optional<SectorPreCommitInfo> precommit_info;

    TipsetKey precommit_tipset;

    InteractiveRandomness seed;
    ChainEpoch seed_epoch;

    proofs::Proof proof;
    boost::optional<CID> message;
    uint64_t invalid_proofs;

    boost::optional<CID> fault_report_message;

    inline std::vector<UnpaddedPieceSize> getExistingPieceSizes() const {
      std::vector<UnpaddedPieceSize> result;

      for (const auto &piece : pieces) {
        result.push_back(piece.piece.size.unpadded());
      }

      return result;
    }

    inline std::vector<PieceInfo> getPieceInfos() const {
      std::vector<PieceInfo> result;

      for (const auto &piece : pieces) {
        result.push_back(piece.piece);
      }

      return result;
    }

    inline std::vector<DealId> getDealIDs() const {
      std::vector<DealId> result;

      for (const auto &piece : pieces) {
        if (piece.deal_info) {
          result.push_back(piece.deal_info->deal_id);
        }
      }

      return result;
    }

    inline uint64_t sealingPriority() const {
      for (const auto &piece : pieces) {
        if (piece.deal_info.has_value()) {
          return kDealSectorPriority;
        }
      }

      return 0;
    }
  };

  // Epochs
  constexpr int kInteractivePoRepConfidence = 6;

  struct PieceAttributes {
    SectorNumber sector = 0;

    PaddedPieceSize offset;
    UnpaddedPieceSize size;
  };

}  // namespace fc::mining::types

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_COMMON_HPP
