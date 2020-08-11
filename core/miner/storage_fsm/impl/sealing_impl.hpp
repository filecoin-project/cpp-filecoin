/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SEALING_IMPL_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SEALING_IMPL_HPP

#include "miner/storage_fsm/sealing.hpp"

#include "common/logger.hpp"
#include "fsm/fsm.hpp"
#include "miner/storage_fsm/api.hpp"
#include "miner/storage_fsm/sealing_states.hpp"
#include "miner/storage_fsm/selaing_events.hpp"
#include "primitives/address/address.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "sector_storage/manager.hpp"

namespace fc::mining {
  using primitives::ChainEpoch;
  using primitives::SectorNumber;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedPieceSize;
  using proofs::SealRandomness;
  using sector_storage::Manager;
  using sector_storage::PreCommit1Output;

  struct SectorInfo {
    primitives::SectorNumber sector_number;
    std::vector<PieceInfo> pieces;

    SealRandomness ticket;
    ChainEpoch epoch;
    PreCommit1Output precommit1_output;
    uint64_t precommit2_fails;

    CID comm_d;
    CID comm_r;

    CID precommit_message;

    TipsetToken precommit_tipset;

    // TODO: add fields

    std::vector<UnpaddedPieceSize> existingPieceSizes() const;
  };

  using SealingTransition =
      fsm::Transition<SealingEvent, SealingState, SectorInfo>;
  using StorageFSM = fsm::FSM<SealingEvent, SealingState, SectorInfo>;

  class SealingImpl : public Sealing {
   private:
    /**
     * Creates all FSM transitions
     * @return vector of transitions for fsm
     */
    std::vector<SealingTransition> makeFSMTransitions();

    /**
     * @brief Handle event incoming sector info
     * @param info  - current sector info
     * @param event - kIncoming
     * @param from  - kStateUnknown
     * @param to    - kPacking
     */
    void onIncoming(const std::shared_ptr<SectorInfo> &info,
                    SealingEvent event,
                    SealingState from,
                    SealingState to);

    /**
     * @brief Handle event pre commit 1 sector info
     * @param info  - current sector info
     * @param event - kPreCommit1
     * @param from  - kPacking, kPreCommit1Failed, kPreCommit2Failed
     * @param to    - kPreCommit1
     */
    void onPreCommit1(const std::shared_ptr<SectorInfo> &info,
                      SealingEvent event,
                      SealingState from,
                      SealingState to);

    /**
     * @brief Handle event pre commit 2 sector info
     * @param info  - current sector info
     * @param event - kPreCommit2
     * @param from  - kPreCommit1, kPreCommit2Failed
     * @param to    - kPreCommit2
     */
    void onPreCommit2(const std::shared_ptr<SectorInfo> &info,
                      SealingEvent event,
                      SealingState from,
                      SealingState to);

    /**
     * @brief Handle event pre commit sector info
     * @param info  - current sector info
     * @param event - kPreCommit
     * @param from  - kPreCommit2, kPreCommitFail
     * @param to    - kPreCommitting
     */
    void onPreCommit(const std::shared_ptr<SectorInfo> &info,
                     SealingEvent event,
                     SealingState from,
                     SealingState to);

    /**
     * @brief Handle event pre commit waiting sector info
     * @param info  - current sector info
     * @param event - kPreCommitWait
     * @param from  - kPreCommitting
     * @param to    - kPreCommittingWaiting
     */
    void onPreCommitWaiting(const std::shared_ptr<SectorInfo> &info,
                            SealingEvent event,
                            SealingState from,
                            SealingState to);

    struct TicketInfo {
      SealRandomness ticket;
      ChainEpoch epoch;
    };

    outcome::result<TicketInfo> getTicket(
        const std::shared_ptr<SectorInfo> &info);

    outcome::result<std::vector<PieceInfo>> pledgeSector(
        SectorId sector,
        std::vector<UnpaddedPieceSize> existing_piece_sizes,
        gsl::span<const UnpaddedPieceSize> sizes);

    SectorId minerSector(SectorNumber num);

    /** State machine */
    std::shared_ptr<StorageFSM> fsm_;
    std::shared_ptr<SealingApi> api_;

    Address miner_address_;

    common::Logger logger_;
    std::shared_ptr<Manager> sealer_;
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SEALING_IMPL_HPP
