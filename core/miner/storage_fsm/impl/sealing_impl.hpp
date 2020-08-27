/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SEALING_IMPL_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SEALING_IMPL_HPP

#include "miner/storage_fsm/sealing.hpp"

#include "api/api.hpp"
#include "common/logger.hpp"
#include "fsm/fsm.hpp"
#include "miner/storage_fsm/events.hpp"
#include "miner/storage_fsm/selaing_events.hpp"

namespace fc::mining {
  using api::Api;
  using primitives::piece::PaddedPieceSize;
  using sector_storage::Manager;

  using SealingTransition =
      fsm::Transition<SealingEvent, SealingState, SectorInfo>;
  using StorageFSM = fsm::FSM<SealingEvent, SealingState, SectorInfo>;

  /**
   * @note for find sectorinfo
   */
  inline bool operator==(const SectorInfo &lhs, const SectorInfo &rhs) {
    return lhs.sector_number == rhs.sector_number;
  }

  class SealingImpl : public Sealing {
   public:
    outcome::result<void> run() override;

    void stop() override;

    outcome::result<SectorNumber> allocatePiece(
        UnpaddedPieceSize size) override;

    outcome::result<void> sealPiece(UnpaddedPieceSize size,
                                    const PieceData &piece_data,
                                    SectorNumber sector_id,
                                    DealInfo deal) override;

    outcome::result<void> remove(SectorNumber sector_id) override;

    Address getAddress() const override;

    std::vector<SectorNumber> getListSectors() const override;

    outcome::result<std::shared_ptr<SectorInfo>> getSectorInfo(
        SectorNumber id) const override;

    outcome::result<void> forceSectorState(SectorNumber id,
                                           SealingState state) override;

   private:
    outcome::result<void> newSector(SectorNumber id,
                                    RegisteredProof seal_proof_type,
                                    const std::vector<Piece> &pieces);

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

    /**
     * @brief Handle event waiting seed
     * @param info  - current sector info
     * @param event - kWaitSeed
     * @param from  - kPreCommitting, kPreCommittingWait, kCommitFail
     * @param to    - kWaitSeed
     */
    void onWaitSeed(const std::shared_ptr<SectorInfo> &info,
                    SealingEvent event,
                    SealingState from,
                    SealingState to);

    /**
     * @brief Handle event commit
     * @param info  - current sector info
     * @param event - kCommit
     * @param from  - kWaitSeed, kCommitFail, kComputeProofFail
     * @param to    - kCommitting
     */
    void onCommit(const std::shared_ptr<SectorInfo> &info,
                  SealingEvent event,
                  SealingState from,
                  SealingState to);

    /**
     * @brief Handle event wait commit
     * @param info  - current sector info
     * @param event - kCommitWait
     * @param from  - kCommitting
     * @param to    - kCommitWait
     */
    void onCommitWait(const std::shared_ptr<SectorInfo> &info,
                      SealingEvent event,
                      SealingState from,
                      SealingState to);

    /**
     * @brief Handle event finalize
     * @param info  - current sector info
     * @param event - kFinalizeSector
     * @param from  - kCommitWait, kFinalizeFail
     * @param to    - kFinalizeSector
     */
    void onFinalize(const std::shared_ptr<SectorInfo> &info,
                    SealingEvent event,
                    SealingState from,
                    SealingState to);

    /**
     * @brief Handle event seal precommit 1 failed
     * @param info  - current sector info
     * @param event - kSealPreCommit1Failed
     * @param from  - kPreCommit1, kPreCommitting, kCommitFail
     * @param to    - kSealPreCommit1Fail
     */
    void onSealPreCommit1Failed(const std::shared_ptr<SectorInfo> &info,
                                SealingEvent event,
                                SealingState from,
                                SealingState to);

    /**
     * @brief Handle event seal precommit 2 failed
     * @param info  - current sector info
     * @param event - kSealPreCommit1Failed
     * @param from  - kPreCommit2
     * @param to    - kSealPreCommit2Fail
     */
    void onSealPreCommit2Failed(const std::shared_ptr<SectorInfo> &info,
                                SealingEvent event,
                                SealingState from,
                                SealingState to);

    /**
     * @brief Handle event precommit failed
     * @param info  - current sector info
     * @param event - kPreCommitFailed
     * @param from  - kPreCommitting, kPreCommittingWait, kWaitSeed
     * @param to    - kPreCommitFail
     */
    void onPreCommitFailed(const std::shared_ptr<SectorInfo> &info,
                           SealingEvent event,
                           SealingState from,
                           SealingState to);

    /**
     * @brief Handle event compute proof failed
     * @param info  - current sector info
     * @param event - kComputeProofFailed
     * @param from  - kCommitting
     * @param to    - kComputeProofFail
     */
    void onComputeProofFailed(const std::shared_ptr<SectorInfo> &info,
                              SealingEvent event,
                              SealingState from,
                              SealingState to);

    /**
     * @brief Handle event commit failed
     * @param info  - current sector info
     * @param event - kCommitFailed
     * @param from  - kCommitting, kCommitWait
     * @param to    - kCommitFail
     */
    void onCommitFailed(const std::shared_ptr<SectorInfo> &info,
                        SealingEvent event,
                        SealingState from,
                        SealingState to);

    /**
     * @brief Handle event finalize failed
     * @param info  - current sector info
     * @param event - kFinalizeFailed
     * @param from  - kFinalizeSector
     * @param to    - kFinalizeFail
     */
    void onFinalizeFailed(const std::shared_ptr<SectorInfo> &info,
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

    std::unordered_map<SectorNumber, std::shared_ptr<SectorInfo>> sectors_;

    /** State machine */
    std::shared_ptr<StorageFSM> fsm_;

    std::shared_ptr<Api> api_;
    std::shared_ptr<Events> events_;

    Address miner_address_;

    common::Logger logger_;
    std::shared_ptr<Manager> sealer_;
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SEALING_IMPL_HPP
