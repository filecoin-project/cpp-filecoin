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
#include "miner/storage_fsm/precommit_policy.hpp"
#include "miner/storage_fsm/sealing_events.hpp"
#include "miner/storage_fsm/sector_stat.hpp"
#include "primitives/stored_counter/stored_counter.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "storage/buffer_map.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::mining {
  using adt::TokenAmount;
  using api::Api;
  using primitives::piece::PaddedPieceSize;
  using proofs::SealRandomness;
  using sector_storage::Manager;
  using types::PieceInfo;
  using SealingTransition = fsm::
      Transition<SealingEvent, SealingEventContext, SealingState, SectorInfo>;
  using StorageFSM =
      fsm::FSM<SealingEvent, SealingEventContext, SealingState, SectorInfo>;
  using libp2p::protocol::Scheduler;
  using Ticks = libp2p::protocol::Scheduler::Ticks;
  using api::SectorPreCommitOnChainInfo;
  using primitives::Counter;
  using primitives::tipset::TipsetKey;
  using storage::BufferMap;
  using vm::actor::builtin::v0::miner::SectorPreCommitInfo;

  struct Config {
    // 0 = no limit
    uint64_t max_wait_deals_sectors = 0;

    // includes failed, 0 = no limit
    uint64_t max_sealing_sectors = 0;

    // includes failed, 0 = no limit
    uint64_t max_sealing_sectors_for_deals = 0;

    uint64_t wait_deals_delay;  // in milliseconds
  };

  class SealingImpl : public Sealing,
                      public std::enable_shared_from_this<SealingImpl> {
   public:
    SealingImpl(std::shared_ptr<Api> api,
                std::shared_ptr<Events> events,
                const Address &miner_address,
                std::shared_ptr<Counter> counter,
                std::shared_ptr<BufferMap> fsm_kv,
                std::shared_ptr<Manager> sealer,
                std::shared_ptr<PreCommitPolicy> policy,
                std::shared_ptr<boost::asio::io_context> context,
                Ticks ticks = 50);

    outcome::result<void> run() override;

    void stop() override;

    outcome::result<void> fsmLoad();
    void fsmSave(const std::shared_ptr<SectorInfo> &info);

    outcome::result<PieceAttributes> addPieceToAnySector(
        UnpaddedPieceSize size,
        const PieceData &piece_data,
        DealInfo deal) override;

    outcome::result<void> remove(SectorNumber sector_id) override;

    Address getAddress() const override;

    std::vector<std::shared_ptr<const SectorInfo>> getListSectors()
        const override;

    outcome::result<std::shared_ptr<SectorInfo>> getSectorInfo(
        SectorNumber id) const override;

    outcome::result<void> forceSectorState(SectorNumber id,
                                           SealingState state) override;

    outcome::result<void> markForUpgrade(SectorNumber id) override;

    bool isMarkedForUpgrade(SectorNumber id) override;

    outcome::result<void> startPacking(SectorNumber id) override;

    outcome::result<void> pledgeSector() override;

   private:
    struct SectorPaddingResponse {
      SectorNumber sector;
      std::vector<PaddedPieceSize> pads;
    };

    outcome::result<SectorPaddingResponse> getSectorAndPadding(
        UnpaddedPieceSize size);

    outcome::result<void> addPiece(SectorNumber sector_id,
                                   UnpaddedPieceSize size,
                                   const PieceData &piece,
                                   const boost::optional<DealInfo> &deal);

    outcome::result<SectorNumber> newDealSector();

    TokenAmount tryUpgradeSector(SectorPreCommitInfo &params);

    boost::optional<SectorNumber> maybeUpgradableSector();

    /**
     * Creates all FSM transitions
     * @return vector of transitions for fsm
     */
    std::vector<SealingTransition> makeFSMTransitions();

    /**
     * callback for fsm to track activity
     */
    void callbackHandle(
        const std::shared_ptr<SectorInfo> &info,
        SealingEvent event,
        const std::shared_ptr<SealingEventContext> &event_context,
        SealingState from,
        SealingState to);

    /**
     * @brief Handle incoming in kPacking state
     */
    outcome::result<void> handlePacking(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kPreCommit1 state
     */
    outcome::result<void> handlePreCommit1(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kPreCommit2 state
     */
    outcome::result<void> handlePreCommit2(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kPreCommitting state
     */
    outcome::result<void> handlePreCommitting(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kPreCommittingWaiting state
     */
    outcome::result<void> handlePreCommitWaiting(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief  Handle incoming in kWaitSeed state
     */
    outcome::result<void> handleWaitSeed(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kCommitting state
     */
    outcome::result<void> handleCommitting(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kCommitWait state
     */
    outcome::result<void> handleCommitWait(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kFinalizeSector state
     */
    outcome::result<void> handleFinalizeSector(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kProving state
     */
    outcome::result<void> handleProvingSector(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kSealPreCommit1Fail state
     */
    outcome::result<void> handleSealPreCommit1Fail(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kSealPreCommit2Fail state
     */
    outcome::result<void> handleSealPreCommit2Fail(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief  Handle incoming in kPreCommitFail state
     */
    outcome::result<void> handlePreCommitFail(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kComputeProofFail state
     */
    outcome::result<void> handleComputeProofFail(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kCommitFail state
     */
    outcome::result<void> handleCommitFail(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kFinalizeFail state
     */
    outcome::result<void> handleFinalizeFail(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kFaultReported state
     */
    outcome::result<void> handleFaultReported(
        const std::shared_ptr<SectorInfo> &info);

    /**
     * @brief Handle incoming in kRemoving state
     */
    outcome::result<void> handleRemoving(
        const std::shared_ptr<SectorInfo> &info);

    struct TicketInfo {
      SealRandomness ticket;
      ChainEpoch epoch = 0;
    };

    outcome::result<TicketInfo> getTicket(
        const std::shared_ptr<SectorInfo> &info);

    outcome::result<std::vector<PieceInfo>> pledgeSector(
        SectorId sector,
        std::vector<UnpaddedPieceSize> existing_piece_sizes,
        gsl::span<UnpaddedPieceSize> sizes);

    outcome::result<void> newSectorWithPieces(
        SectorNumber sector_id, std::vector<types::Piece> &pieces);

    SectorId minerSector(SectorNumber num);

    mutable std::mutex sectors_mutex_;
    std::unordered_map<SectorNumber, std::shared_ptr<SectorInfo>> sectors_;

    struct UnsealedSectorInfo {
      uint64_t deals_number;
      // stored should always equal sum of piece_sizes.padded()
      PaddedPieceSize stored;
      std::vector<UnpaddedPieceSize> piece_sizes;
    };

    std::unordered_map<SectorNumber, UnsealedSectorInfo> unsealed_sectors_;
    std::shared_mutex unsealed_mutex_;

    std::set<SectorNumber> to_upgrade_;
    std::shared_mutex upgrade_mutex_;

    /** State machine */
    std::shared_ptr<boost::asio::io_context> context_;
    std::shared_ptr<StorageFSM> fsm_;

    std::shared_ptr<Api> api_;
    std::shared_ptr<Events> events_;

    std::shared_ptr<PreCommitPolicy> policy_;

    std::shared_ptr<Counter> counter_;
    std::shared_ptr<BufferMap> fsm_kv_;

    std::shared_ptr<SectorStat> stat_;

    Address miner_address_;

    common::Logger logger_;
    std::shared_ptr<Manager> sealer_;

    Config config_;
    std::shared_ptr<Scheduler> scheduler_;
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SEALING_IMPL_HPP
