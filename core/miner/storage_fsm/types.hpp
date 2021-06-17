/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/sealing_states.hpp"
#include "miner/storage_fsm/types.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "primitives/types.hpp"
#include "sector_storage/manager.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"

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
  using sector_storage::Range;
  using vm::actor::builtin::types::market::DealProposal;
  using vm::actor::builtin::types::miner::SectorPreCommitInfo;

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
  CBOR_TUPLE(DealSchedule, start_epoch, end_epoch)

  inline bool operator==(const DealSchedule &lhs, const DealSchedule &rhs) {
    return lhs.start_epoch == rhs.start_epoch && lhs.end_epoch == rhs.end_epoch;
  }

  /** DealInfo is a tuple of deal identity and its schedule */
  struct DealInfo {
    boost::optional<CID> publish_cid;
    DealId deal_id;
    boost::optional<DealProposal> deal_proposal;
    DealSchedule deal_schedule;
    bool is_keep_unsealed;
  };
  CBOR_TUPLE(DealInfo,
             publish_cid,
             deal_id,
             deal_proposal,
             deal_schedule,
             is_keep_unsealed)

  inline bool operator==(const DealInfo &lhs, const DealInfo &rhs) {
    return lhs.publish_cid == rhs.publish_cid && lhs.deal_id == rhs.deal_id
           && lhs.deal_schedule == rhs.deal_schedule
           && lhs.is_keep_unsealed == rhs.is_keep_unsealed;
  }

  struct Piece {
    PieceInfo piece;
    boost::optional<DealInfo> deal_info;
  };
  CBOR_TUPLE(Piece, piece, deal_info)

  struct SectorInfo {
    SealingState state;

    SectorNumber sector_number;
    RegisteredSealProof sector_type;
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

    std::vector<CbCid> precommit_tipset;

    InteractiveRandomness seed;
    ChainEpoch seed_epoch;

    proofs::Proof proof;
    boost::optional<CID> message;
    uint64_t invalid_proofs;

    boost::optional<CID> fault_report_message;

    SealingState return_state;

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

    inline std::vector<Range> keepUnsealedRanges(bool is_invert = false) const {
      std::vector<Range> res;

      UnpaddedPieceSize offset(0);
      for (const auto &piece : pieces) {
        auto piece_size = piece.piece.size.unpadded();

        offset += piece_size;
        if (not piece.deal_info.has_value()) {
          continue;
        }
        if (piece.deal_info.get().is_keep_unsealed == is_invert) {
          continue;
        }

        res.push_back(Range{
            .offset = UnpaddedPieceSize(offset - piece_size),
            .size = piece_size,
        });
      }

      return res;
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
  CBOR_TUPLE(SectorInfo,
             state,
             sector_number,
             sector_type,
             pieces,
             ticket,
             ticket_epoch,
             precommit1_output,
             precommit2_fails,
             comm_d,
             comm_r,
             precommit_message,
             precommit_deposit,
             precommit_info,
             precommit_tipset,
             seed,
             seed_epoch,
             proof,
             message,
             invalid_proofs,
             fault_report_message,
             return_state)

  struct PieceAttributes {
    SectorNumber sector = 0;

    PaddedPieceSize offset;
    UnpaddedPieceSize size;
  };

  inline bool operator==(const PieceAttributes &lhs,
                         const PieceAttributes &rhs) {
    return lhs.sector == rhs.sector && lhs.offset == rhs.offset
           && lhs.size == rhs.size;
  }

}  // namespace fc::mining::types
