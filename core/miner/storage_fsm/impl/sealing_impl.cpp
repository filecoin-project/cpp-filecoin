/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/sealing_impl.hpp"

#include "common/bitsutil.hpp"
#include "host/context/impl/host_context_impl.hpp"
#include "miner/storage_fsm/impl/basic_precommit_policy.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"

#define FSM_SEND(info, event) OUTCOME_EXCEPT(fsm_->send(info, event))

#define CALLBACK_ACTION \
  [](auto info, auto event, auto from, auto to) { event->apply(info); }

namespace fc::mining {
  using types::Piece;

  SealingImpl::SealingImpl(std::shared_ptr<boost::asio::io_context> context)
      : context_(std::move(context)) {
    std::shared_ptr<host::HostContext> fsm_context =
        std::make_shared<host::HostContextImpl>(context_);
    fsm_ = std::make_shared<StorageFSM>(makeFSMTransitions(), fsm_context);
    fsm_->setAnyChangeAction([this](auto info, auto event, auto from, auto to) {
      callbackHandle(info, event, from, to);
    });
  }

  uint64_t getDealPerSectorLimit(SectorSize size) {
    if (size < (uint64_t(64) << 30)) {
      return 256;
    }
    return 512;
  }

  outcome::result<void> SealingImpl::run() {
    // TODO: restart fsm
    return outcome::success();
  }

  void SealingImpl::stop() {
    // TODO: log it
    fsm_->stop();
  }

  outcome::result<PieceAttributes> SealingImpl::addPieceToAnySector(
      UnpaddedPieceSize size, const PieceData &piece_data, DealInfo deal) {
    // TODO: Log it

    if (primitives::piece::paddedSize(size) != size) {
      return outcome::success();  // TODO: error
    }

    auto sector_size = sealer_->getSectorSize();
    if (size > PaddedPieceSize(sector_size).unpadded()) {
      return outcome::success();  // TODO: error
    }

    bool is_start_packing = false;
    PieceAttributes piece;
    piece.size = size;

    {
      std::unique_lock lock(unsealed_mutex_);
      OUTCOME_TRY(sector_and_padding, getSectorAndPadding(size));

      piece.sector = sector_and_padding.sector;

      PieceData zero_file("/dev/zero");
      for (const auto &pad : sector_and_padding.pads) {
        OUTCOME_TRY(addPiece(
            sector_and_padding.sector, pad.unpadded(), zero_file, boost::none));
      }

      piece.offset = unsealed_sectors_[sector_and_padding.sector].stored;

      OUTCOME_TRY(addPiece(sector_and_padding.sector, size, piece_data, deal));

      is_start_packing =
          unsealed_sectors_[sector_and_padding.sector].deals_number
          >= getDealPerSectorLimit(sector_size);
    }

    if (is_start_packing) {
      OUTCOME_TRY(startPacking(piece.sector));
    }

    return piece;
  }

  outcome::result<void> SealingImpl::remove(SectorNumber sector_id) {
    auto sector = sectors_.find(sector_id);
    if (sector == sectors_.end()) {
      return outcome::success();  // TODO: Error
    }

    return fsm_->send(sector->second, std::make_shared<SectorRemoveEvent>());
  }

  Address SealingImpl::getAddress() const {
    return miner_address_;
  }

  std::vector<SectorNumber> SealingImpl::getListSectors() const {
    std::vector<SectorNumber> keys = {};
    for (const auto &[key, value] : sectors_) {
      keys.push_back(key);
    }
    return keys;
  }

  outcome::result<std::shared_ptr<SectorInfo>> SealingImpl::getSectorInfo(
      SectorNumber id) const {
    const auto &maybe_sector{sectors_.find(id)};
    if (maybe_sector == sectors_.cend()) {
      return outcome::success();  // TODO: error
    }
    return maybe_sector->second;
  }

  outcome::result<void> SealingImpl::forceSectorState(SectorNumber id,
                                                      SealingState state) {
    // TODO: send fsm event
    return outcome::success();
  }

  outcome::result<void> SealingImpl::markForUpgrade(SectorNumber id) {
    std::unique_lock lock(upgrade_mutex_);

    if (to_upgrade_.find(id) != to_upgrade_.end()) {
      return outcome::success();  // TODO: ERROR
    }

    OUTCOME_TRY(sector_info, getSectorInfo(id));

    OUTCOME_TRY(state,
                fsm_->get(sector_info));  // TODO: maybe save state in info

    if (state != SealingState::kProving) {
      return outcome::success();  // TODO: ERROR
    }

    if (sector_info->pieces.size() != 1) {
      return outcome::success();  // TODO: ERROR
    }

    if (sector_info->pieces[0].deal_info.has_value()) {
      return outcome::success();  // TODO: ERROR
    }

    // TODO: more checks to match actor constraints
    to_upgrade_.insert(id);

    return outcome::success();
  }

  bool SealingImpl::isMarkedForUpgrade(SectorNumber id) {
    std::shared_lock lock(upgrade_mutex_);
    return to_upgrade_.find(id) != to_upgrade_.end();
  }

  outcome::result<void> SealingImpl::pledgeSector() {
    // TODO: Implement me
    return outcome::success();
  }

  outcome::result<void> SealingImpl::startPacking(SectorNumber id) {
    // TODO: log it
    auto sector_info = sectors_.find(id);
    if (sector_info == sectors_.end()) {
      return outcome::success();  // TODO: ERROR
    }

    OUTCOME_TRY(fsm_->send(sector_info->second,
                           std::make_shared<SectorStartPackingEvent>()));

    {
      std::lock_guard lock(unsealed_mutex_);
      unsealed_sectors_.erase(id);
    }

    return outcome::success();
  }

  outcome::result<SealingImpl::SectorPaddingResponse>
  SealingImpl::getSectorAndPadding(UnpaddedPieceSize size) {
    auto sector_size = sealer_->getSectorSize();

    for (const auto &[key, value] : unsealed_sectors_) {
      auto pads =
          proofs::Proofs::GetRequiredPadding(value.stored, size.padded());
      if (value.stored + size.padded() + pads.size < sector_size) {
        return SectorPaddingResponse{
            .sector = key,
            .pads = std::move(pads.pads),
        };
      }
    }

    OUTCOME_TRY(new_sector, newDealSector());

    unsealed_sectors_[new_sector] = UnsealedSectorInfo{
        .deals_number = 0,
        .stored = PaddedPieceSize(0),
        .piece_sizes = {},
    };

    return SectorPaddingResponse{
        .sector = new_sector,
        .pads = {},
    };
  }

  outcome::result<void> SealingImpl::addPiece(
      SectorNumber sector_id,
      UnpaddedPieceSize size,
      const PieceData &piece,
      const boost::optional<DealInfo> &deal) {
    // TODO: log it
    OUTCOME_TRY(piece_info,
                sealer_->addPiece(minerSector(sector_id),
                                  unsealed_sectors_[sector_id].piece_sizes,
                                  size,
                                  piece));

    Piece new_piece{
        .piece = piece_info,
        .deal_info = deal,
    };

    std::shared_ptr<SectorAddPieceEvent> event =
        std::make_shared<SectorAddPieceEvent>();
    event->piece = new_piece;
    auto info = sectors_[sector_id];
    OUTCOME_TRY(fsm_->send(info, event));

    auto unsealed_info = unsealed_sectors_.find(sector_id);
    if (deal) {
      unsealed_info->second.deals_number++;
    }
    unsealed_info->second.stored += new_piece.piece.size;
    unsealed_info->second.piece_sizes.push_back(
        new_piece.piece.size.unpadded());

    return outcome::success();
  }

  outcome::result<SectorNumber> SealingImpl::newDealSector() {
    if (config_.max_sealing_sectors_for_deals > 0) {
      if (stat_->currentSealing() > config_.max_sealing_sectors_for_deals) {
        return outcome::success();  // TODO: ERROR
      }
    }

    if (config_.max_wait_deals_sectors > 0
        && unsealed_sectors_.size() >= config_.max_wait_deals_sectors) {
      for (size_t i = 0; i < 10; i++) {
        if (i) {
          // TODO: wait 1 second
        }
        uint64_t best_id;
        {
          std::lock_guard lock(unsealed_mutex_);

          if (unsealed_sectors_.empty()) {
            break;
          }

          auto first_sector{unsealed_sectors_.cbegin()};
          best_id = first_sector->first;
          PaddedPieceSize most_stored = first_sector->second.stored;

          for (auto iter = ++(unsealed_sectors_.cbegin());
               iter != unsealed_sectors_.cend();
               iter++) {
            if (iter->second.stored > most_stored) {
              most_stored = iter->second.stored;
              best_id = iter->first;
            }
          }
        }
        auto maybe_error = startPacking(best_id);
        if (maybe_error.has_error()) {
          // TODO: log it
        }
      }
    }

    // TODO: log it
    OUTCOME_TRY(sector_id, counter_->next());

    auto sector = std::make_shared<SectorInfo>();
    OUTCOME_TRY(fsm_->begin(sector, SealingState::kStateUnknown));
    sectors_[sector_id] = sector;
    std::shared_ptr<SectorStartEvent> event =
        std::make_shared<SectorStartEvent>();
    event->sector_id = sector_id;
    OUTCOME_TRYA(event->seal_proof_type,
                 primitives::sector::sealProofTypeFromSectorSize(
                     sealer_->getSectorSize()));
    OUTCOME_TRY(fsm_->send(sector, event));

    // TODO: Timer for start packing

    return sector_id;
  }

  std::vector<UnpaddedPieceSize> filler(UnpaddedPieceSize in) {
    uint64_t to_fill = in.padded();

    uint64_t pieces_size = common::countSetBits(to_fill);

    std::vector<UnpaddedPieceSize> out;
    for (size_t i = 0; i < pieces_size; ++i) {
      uint64_t next = common::countTrailingZeros(to_fill);
      uint64_t piece_size = uint64_t(1) << next;

      to_fill ^= piece_size;

      out.push_back(PaddedPieceSize(piece_size).unpadded());
    }
    return out;
  }

  std::vector<SealingTransition> SealingImpl::makeFSMTransitions() {
    return {
        // Main pipeline
        SealingTransition(std::make_shared<SectorStartEvent>())
            .from(SealingState::kStateUnknown)
            .to(SealingState::kWaitDeals)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorStartWithPiecesEvent>())
            .from(SealingState::kStateUnknown)
            .to(SealingState::kPacking)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorAddPieceEvent>())
            .from(SealingState::kWaitDeals)
            .toSameState()
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorStartPackingEvent>())
            .from(SealingState::kWaitDeals)
            .to(SealingState::kPacking)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorPackedEvent>())
            .from(SealingState::kPacking)
            .to(SealingState::kPreCommit1)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorPreCommit1Event>())
            .from(SealingState::kPreCommit1)
            .to(SealingState::kPreCommit2)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorSealPreCommit1FailedEvent>())
            .fromMany(SealingState::kPreCommit1, SealingState::kPreCommitting)
            .to(SealingState::kSealPreCommit1Fail)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorPackingFailedEvent>())
            .fromMany(SealingState::kPreCommit1, SealingState::kPreCommit2)
            .to(SealingState::kPackingFail)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorPreCommit2Event>())
            .from(SealingState::kPreCommit2)
            .to(SealingState::kPreCommitting)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorSealPreCommit2FailedEvent>())
            .from(SealingState::kPreCommit2)
            .to(SealingState::kSealPreCommit2Fail)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorPreCommittedEvent>())
            .from(SealingState::kPreCommitting)
            .to(SealingState::kPreCommittingWait)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorChainPreCommitFailedEvent>())
            .fromMany(SealingState::kPreCommitting,
                      SealingState::kPreCommittingWait,
                      SealingState::kWaitSeed,
                      SealingState::kCommitFail)
            .to(SealingState::kPreCommitFail)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorPreCommitLandedEvent>())
            .fromMany(SealingState::kPreCommitting,
                      SealingState::kPreCommittingWait,
                      SealingState::kPreCommitFail)
            .to(SealingState::kWaitSeed)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorSeedReadyEvent>())
            .fromMany(SealingState::kWaitSeed, SealingState::kCommitting)
            .to(SealingState::kCommitting)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorCommittedEvent>())
            .from(SealingState::kCommitting)
            .to(SealingState::kCommitWait)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorComputeProofFailedEvent>())
            .from(SealingState::kCommitting)
            .to(SealingState::kComputeProofFail)
            .action(CALLBACK_ACTION),
        // NOTE: there is not call the apply function
        SealingTransition(std::make_shared<SectorSealPreCommit1FailedEvent>())
            .from(SealingState::kCommitting)
            .to(SealingState::kSealPreCommit1Fail),
        SealingTransition(std::make_shared<SectorCommitFailedEvent>())
            .fromMany(SealingState::kCommitting, SealingState::kCommitWait)
            .to(SealingState::kCommitFail)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRetryCommitWaitEvent>())
            .fromMany(SealingState::kCommitting, SealingState::kCommitFail)
            .to(SealingState::kCommitWait)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorProvingEvent>())
            .from(SealingState::kCommitWait)
            .to(SealingState::kFinalizeSector)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorFinalizedEvent>())
            .from(SealingState::kFinalizeSector)
            .to(SealingState::kProving)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorFinalizeFailedEvent>())
            .from(SealingState::kFinalizeSector)
            .to(SealingState::kFinalizeFail)
            .action(CALLBACK_ACTION),

        SealingTransition(std::make_shared<SectorRetrySealPreCommit1Event>())
            .fromMany(SealingState::kSealPreCommit1Fail,
                      SealingState::kSealPreCommit2Fail,
                      SealingState::kPreCommitFail,
                      SealingState::kComputeProofFail,
                      SealingState::kCommitFail)
            .to(SealingState::kPreCommit1)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRetrySealPreCommit2Event>())
            .from(SealingState::kSealPreCommit2Fail)
            .to(SealingState::kPreCommit2)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRetryPreCommitEvent>())
            .fromMany(SealingState::kPreCommitFail, SealingState::kCommitFail)
            .to(SealingState::kPreCommitting)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRetryWaitSeedEvent>())
            .fromMany(SealingState::kPreCommitFail, SealingState::kCommitFail)
            .to(SealingState::kWaitSeed)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRetryComputeProofEvent>())
            .fromMany(SealingState::kComputeProofFail,
                      SealingState::kCommitFail)
            .to(SealingState::kCommitting)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRetryInvalidProofEvent>())
            .from(SealingState::kCommitFail)
            .to(SealingState::kCommitting)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRetryPreCommitWaitEvent>())
            .from(SealingState::kCommitFail)
            .to(SealingState::kPreCommittingWait)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRetryFinalizeEvent>())
            .from(SealingState::kFinalizeFail)
            .to(SealingState::kFinalizeSector)
            .action(CALLBACK_ACTION),

        SealingTransition(std::make_shared<SectorFaultReportedEvent>())
            .fromMany(SealingState::kProving, SealingState::kFaulty)
            .to(SealingState::kFaultReported)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorFaultyEvent>())
            .from(SealingState::kProving)
            .to(SealingState::kFaulty)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRemoveEvent>())
            .from(SealingState::kProving)
            .to(SealingState::kRemoving)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRemovedEvent>())
            .from(SealingState::kRemoving)
            .to(SealingState::kRemoved)
            .action(CALLBACK_ACTION),
        SealingTransition(std::make_shared<SectorRemoveFailedEvent>())
            .from(SealingState::kRemoving)
            .to(SealingState::kRemoveFail)
            .action(CALLBACK_ACTION),
    };
  }

  void SealingImpl::callbackHandle(const std::shared_ptr<SectorInfo> &info,
                                   EventPtr event,
                                   SealingState from,
                                   SealingState to) {
    auto maybe_error = [&]() -> outcome::result<void> {
      switch (to) {
        case SealingState::kWaitDeals:
          logger_->info("Waiting for deals {}", info->sector_number);
          return outcome::success();
        case SealingState::kPacking:
          return handlePacking(info);
        case SealingState::kPreCommit1:
          return handlePreCommit1(info);
        case SealingState::kPreCommit2:
          return handlePreCommit2(info);
        case SealingState::kPreCommitting:
          return handlePreCommitting(info);
        case SealingState::kPreCommittingWait:
          return handlePreCommitWaiting(info);
        case SealingState::kWaitSeed:
          return handleWaitSeed(info);
        case SealingState::kCommitting:
          return handleCommitting(info);
        case SealingState::kCommitWait:
          return handleCommitWait(info);
        case SealingState::kFinalizeSector:
          return handleFinalizeSector(info);

        case SealingState::kSealPreCommit1Fail:
          return handleSealPreCommit1Fail(info);
        case SealingState::kSealPreCommit2Fail:
          return handleSealPreCommit2Fail(info);
        case SealingState::kPreCommitFail:
          return handlePreCommitFail(info);
        case SealingState::kComputeProofFail:
          return handleComputeProofFail(info);
        case SealingState::kCommitFail:
          return handleCommitFail(info);
        case SealingState::kFinalizeFail:
          return handleFinalizeFail(info);

        case SealingState::kProving:
          return handleProvingSector(info);
        case SealingState::kRemoving:
          return handleRemoving(info);
        case SealingState::kRemoved:
          return outcome::success();

        case SealingState::kFaulty:
          return outcome::success();
        case SealingState::kFaultReported:
          return handleFaultReported(info);

        case SealingState::kStateUnknown:
          logger_->error("sector update with undefined state!");
          return outcome::success();
        default:
          logger_->error("Unknown state {}", to);
          return outcome::success();
      }
    }();
    if (maybe_error.has_error()) {
      logger_->error("Unhandled sector error ({}): {}",
                     info->sector_number,
                     maybe_error.error());
    }
  }

  outcome::result<void> SealingImpl::handlePacking(
      const std::shared_ptr<SectorInfo> &info) {
    logger_->info("Performing filling up rest of the sector",
                  info->sector_number);

    UnpaddedPieceSize allocated(0);
    for (const auto &piece : info->pieces) {
      allocated += piece.piece.size.unpadded();
    }

    auto ubytes = PaddedPieceSize(sealer_->getSectorSize()).unpadded();

    if (allocated > ubytes) {
      logger_->error("too much data in sector: {} > {}", allocated, ubytes);
      return outcome::success();
    }

    auto filler_sizes = filler(UnpaddedPieceSize(ubytes - allocated));

    if (!filler_sizes.empty()) {
      logger_->warn("Creating {} filler pieces for sector {}",
                    filler_sizes.size(),
                    info->sector_number);
    }

    OUTCOME_TRY(result,
                pledgeSector(minerSector(info->sector_number),
                             info->getExistingPieceSizes(),
                             filler_sizes));

    std::shared_ptr<SectorPackedEvent> event =
        std::make_shared<SectorPackedEvent>();
    event->filler_pieces = std::move(result);

    return fsm_->send(info, event);
  }

  outcome::result<std::vector<PieceInfo>> SealingImpl::pledgeSector(
      SectorId sector,
      std::vector<UnpaddedPieceSize> existing_piece_sizes,
      gsl::span<const UnpaddedPieceSize> sizes) {
    if (sizes.empty()) {
      return outcome::success();
    }

    std::string existing_piece_str = "empty";
    if (!existing_piece_sizes.empty()) {
      existing_piece_str = std::to_string(existing_piece_sizes[0]);
      for (size_t i = 1; i < existing_piece_sizes.size(); ++i) {
        existing_piece_str += ", ";
        existing_piece_str += std::to_string(existing_piece_sizes[i]);
      }
    }

    logger_->info("Pledge " + primitives::sector_file::sectorName(sector)
                  + ", contains " + existing_piece_str);

    std::vector<PieceInfo> result;

    PieceData zero_file("/dev/zero");
    for (const auto &size : sizes) {
      OUTCOME_TRY(
          piece_info,
          sealer_->addPiece(sector, existing_piece_sizes, size, zero_file));

      existing_piece_sizes.push_back(size);

      result.push_back(piece_info);
    }

    return std::move(result);
  }

  SectorId SealingImpl::minerSector(SectorNumber num) {
    auto miner_id = miner_address_.getId();

    return SectorId{
        .miner = miner_id,
        .sector = num,
    };
  }

  outcome::result<void> SealingImpl::handlePreCommit1(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: check Packing

    logger_->info("Performing {} sector replication", info->sector_number);

    auto maybe_ticket = getTicket(info);
    if (maybe_ticket.has_error()) {
      logger_->error("Get ticket error: {}", maybe_ticket.error().message());
      return fsm_->send(info,
                        std::make_shared<SectorSealPreCommit1FailedEvent>());
    }

    // TODO: add check priority
    auto maybe_result =
        sealer_->sealPreCommit1(minerSector(info->sector_number),
                                maybe_ticket.value().ticket,
                                info->getPieceInfos());

    if (maybe_result.has_error()) {
      logger_->error("Seal pre commit 1 error: {}",
                     maybe_result.error().message());
      return fsm_->send(info,
                        std::make_shared<SectorSealPreCommit1FailedEvent>());
    }

    std::shared_ptr<SectorPreCommit1Event> event =
        std::make_shared<SectorPreCommit1Event>();
    event->precommit1_output = maybe_result.value();
    event->ticket = maybe_ticket.value().ticket;
    event->epoch = maybe_ticket.value().epoch;

    return fsm_->send(info, event);
  }

  outcome::result<void> SealingImpl::handlePreCommit2(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: add check priority
    auto maybe_cid = sealer_->sealPreCommit2(minerSector(info->sector_number),
                                             info->precommit1_output);
    if (maybe_cid.has_error()) {
      logger_->error("Seal pre commit 2 error: {}",
                     maybe_cid.error().message());
      return fsm_->send(info,
                        std::make_shared<SectorSealPreCommit2FailedEvent>());
    }

    std::shared_ptr<SectorPreCommit2Event> event =
        std::make_shared<SectorPreCommit2Event>();
    event->unsealed = maybe_cid.value().unsealed_cid;
    event->sealed = maybe_cid.value().sealed_cid;

    return fsm_->send(info, event);
  }

  outcome::result<void> SealingImpl::handlePreCommitting(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());
    OUTCOME_TRY(key, head.makeKey());

    OUTCOME_TRY(worker_addr, api_->StateMinerWorker(miner_address_, key));

    // TODO: check Precommit

    // TODO: check Policy

    // TODO: CBOR params

    adt::TokenAmount deposit(0);  // TODO: get deposit

    logger_->info("submitting precommit for sector: {}", info->sector_number);
    auto maybe_signed_msg = api_->MpoolPushMessage(vm::message::UnsignedMessage(
        worker_addr,
        miner_address_,
        0,
        0,
        1,
        1000000,
        vm::actor::builtin::miner::PreCommitSector::Number,
        {}));

    if (maybe_signed_msg.has_error()) {
      logger_->error("pushing message to mpool: {}",
                     maybe_signed_msg.error().message());
      return fsm_->send(info,
                        std::make_shared<SectorChainPreCommitFailedEvent>());
    }

    std::shared_ptr<SectorPreCommittedEvent> event =
        std::make_shared<SectorPreCommittedEvent>();
    event->precommit_message = maybe_signed_msg.value().getCid();
    event->precommit_deposit = deposit;
    event->precommit_info = {};  // TODO: params

    return fsm_->send(info, event);
  }

  outcome::result<void> SealingImpl::handlePreCommitWaiting(
      const std::shared_ptr<SectorInfo> &info) {
    if (!info->precommit_message) {
      logger_->error("precommit message was nil");
      return fsm_->send(info,
                        std::make_shared<SectorChainPreCommitFailedEvent>());
    }

    logger_->info("Sector precommitted: {}", info->sector_number);
    OUTCOME_TRY(channel, api_->StateWaitMsg(info->precommit_message.value()));

    auto maybe_lookup = channel.waitSync();
    if (maybe_lookup.has_error()) {
      logger_->error("sector precommit failed: {}",
                     maybe_lookup.error().message());
      return fsm_->send(info,
                        std::make_shared<SectorChainPreCommitFailedEvent>());
    }

    if (maybe_lookup.value().receipt.exit_code != vm::VMExitCode::kOk) {
      logger_->error("sector precommit failed: exit code is {}",
                     maybe_lookup.value().receipt.exit_code);
      return fsm_->send(info,
                        std::make_shared<SectorChainPreCommitFailedEvent>());
    }

    OUTCOME_TRY(tipset_key, maybe_lookup.value().tipset.makeKey());

    std::shared_ptr<SectorPreCommitLandedEvent> event =
        std::make_shared<SectorPreCommitLandedEvent>();
    event->tipset_key = tipset_key;

    return fsm_->send(info, event);
  }

  outcome::result<void> SealingImpl::handleWaitSeed(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: get pre commit info from api

    auto random_height = vm::actor::builtin::miner::
        kPreCommitChallengeDelay;  // TODO: add height from pre commit info

    auto maybe_error = events_->chainAt(
        [=](const Tipset &,
            ChainEpoch current_height) -> outcome::result<void> {
          // TODO: CBOR miner address

          OUTCOME_TRY(head, api_->ChainHead());
          OUTCOME_TRY(tipset_key, head.makeKey());

          auto maybe_randomness = api_->ChainGetRandomnessFromBeacon(
              tipset_key,
              crypto::randomness::DomainSeparationTag::
                  InteractiveSealChallengeSeed,
              random_height,
              {});
          if (maybe_randomness.has_error()) {
            OUTCOME_TRY(fsm_->send(
                info, std::make_shared<SectorChainPreCommitFailedEvent>()));
            return maybe_randomness.error();
          }

          std::shared_ptr<SectorSeedReadyEvent> event =
              std::make_shared<SectorSeedReadyEvent>();

          event->seed = maybe_randomness.value();
          event->epoch = random_height;

          return fsm_->send(info, event);
        },
        [=](const Tipset &token) -> outcome::result<void> {
          logger_->warn("revert in interactive commit sector step");
          // TODO: cancel running and restart
          return outcome::success();
        },
        types::kInteractivePoRepConfidence,
        random_height);

    if (maybe_error.has_error()) {
      logger_->warn("waitForPreCommitMessage ChainAt errored: {}",
                    maybe_error.error().message());
    }

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleCommitting(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: check that message exist

    logger_->info("scheduling seal proof computation...");

    logger_->info(
        "commit {} sector; ticket(epoch): {}({});"
        "seed(epoch): {}({}); ticket(epoch): {}({})",
        info->sector_number,
        info->ticket,
        info->ticket_epoch,
        info->seed,
        info->seed_epoch);

    if (!(info->comm_d && info->comm_r)) {
      logger_->error("sector had nil commR or commD");
      return fsm_->send(info, std::make_shared<SectorCommitFailedEvent>());
    }

    sector_storage::SectorCids cids{
        .sealed_cid = info->comm_r.get(),
        .unsealed_cid = info->comm_d.get(),
    };

    // TODO: add check priority
    auto maybe_commit_1_output =
        sealer_->sealCommit1(minerSector(info->sector_number),
                             info->ticket,
                             info->seed,
                             info->getPieceInfos(),
                             cids);
    if (maybe_commit_1_output.has_error()) {
      logger_->error("computing seal proof failed(1): {}",
                     maybe_commit_1_output.error().message());
      return fsm_->send(info,
                        std::make_shared<SectorComputeProofFailedEvent>());
    }

    auto maybe_proof = sealer_->sealCommit2(minerSector(info->sector_number),
                                            maybe_commit_1_output.value());
    if (maybe_proof.has_error()) {
      logger_->error("computing seal proof failed(2): {}",
                     maybe_proof.error().message());
      return fsm_->send(info,
                        std::make_shared<SectorComputeProofFailedEvent>());
    }

    OUTCOME_TRY(head, api_->ChainHead());

    // TODO: check Commit

    // TODO: maybe split into 2 states here

    // TODO: CBOR params

    OUTCOME_TRY(tipset_key, head.makeKey());

    OUTCOME_TRY(worker_addr,
                api_->StateMinerWorker(miner_address_, tipset_key));

    OUTCOME_TRY(collateral,
                api_->StateMinerInitialPledgeCollateral(
                    miner_address_, info->sector_number, tipset_key));

    // TODO: collateral manipulation

    auto maybe_signed_msg = api_->MpoolPushMessage(vm::message::UnsignedMessage(
        worker_addr,
        miner_address_,
        0,
        collateral,
        1,
        1000000,
        vm::actor::builtin::miner::ProveCommitSector::Number,
        {}));

    if (maybe_signed_msg.has_error()) {
      logger_->error("pushing message to mpool: {}",
                     maybe_signed_msg.error().message());
      return fsm_->send(info, std::make_shared<SectorCommitFailedEvent>());
    }

    std::shared_ptr<SectorCommittedEvent> event =
        std::make_shared<SectorCommittedEvent>();
    event->proof = maybe_proof.value();
    event->message = maybe_signed_msg.value().getCid();

    return fsm_->send(info, event);
  }

  outcome::result<void> SealingImpl::handleCommitWait(
      const std::shared_ptr<SectorInfo> &info) {
    if (!info->message) {
      logger_->error(
          "sector {} entered commit wait state without a message cid",
          info->sector_number);
      return fsm_->send(info, std::make_shared<SectorCommitFailedEvent>());
    }

    OUTCOME_TRY(channel, api_->StateWaitMsg(info->message.get()));

    auto maybe_message_lookup = channel.waitSync();

    if (maybe_message_lookup.has_error()) {
      logger_->error("failed to wait for porep inclusion: {}",
                     maybe_message_lookup.error().message());
      return fsm_->send(info, std::make_shared<SectorCommitFailedEvent>());
    }

    if (maybe_message_lookup.value().receipt.exit_code != vm::VMExitCode::kOk) {
      logger_->error(
          "submitting sector proof failed with code {}, message cid: {}",
          maybe_message_lookup.value().receipt.exit_code,
          info->message.get());
      return fsm_->send(info, std::make_shared<SectorCommitFailedEvent>());
    }

    OUTCOME_TRY(tipset_key, maybe_message_lookup.value().tipset.makeKey());

    auto maybe_error = api_->StateSectorGetInfo(
        miner_address_, info->sector_number, tipset_key);

    if (maybe_error.has_error()) {
      logger_->error(
          "proof validation failed, sector not found in sector set after cron: "
          "{}",
          maybe_error.error().message());
      return fsm_->send(info, std::make_shared<SectorCommitFailedEvent>());
    }

    return fsm_->send(info, std::make_shared<SectorProvingEvent>());
  }

  outcome::result<void> SealingImpl::handleFinalizeSector(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: Maybe wait for some finality

    auto maybe_error =
        sealer_->finalizeSector(minerSector(info->sector_number));
    if (maybe_error.has_error()) {
      logger_->error("finalize sector: {}", maybe_error.error().message());
      return fsm_->send(info, std::make_shared<SectorFinalizeFailedEvent>());
    }

    return fsm_->send(info, std::make_shared<SectorFinalizedEvent>());
  }

  outcome::result<void> SealingImpl::handleProvingSector(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: track sector health / expiration

    logger_->info("Proving sector {}", info->sector_number);

    // TODO: release unsealed

    // TODO: Watch termination
    // TODO: Auto-extend if set

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleSealPreCommit1Fail(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: wait some time

    return fsm_->send(info, std::make_shared<SectorRetrySealPreCommit1Event>());
  }

  outcome::result<void> SealingImpl::handleSealPreCommit2Fail(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: wait some time

    if (info->precommit2_fails > 1) {
      return fsm_->send(info,
                        std::make_shared<SectorRetrySealPreCommit1Event>());
    }

    return fsm_->send(info, std::make_shared<SectorRetrySealPreCommit2Event>());
  }

  outcome::result<void> SealingImpl::handlePreCommitFail(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());

    // TODO: check precommit

    // TODO: check precommitted

    if (info->precommit_message) {
      logger_->warn(
          "retrying precommit even though the message failed to apply");
    }

    // TODO: wait some time

    return fsm_->send(info, std::make_shared<SectorRetryPreCommitEvent>());
  }

  outcome::result<void> SealingImpl::handleComputeProofFail(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: Check sector files

    // TODO: wait some time

    if (info->invalid_proofs > 1) {
      logger_->error("consecutive compute fails");
      return fsm_->send(info,
                        std::make_shared<SectorSealPreCommit1FailedEvent>());
    }

    return fsm_->send(info, std::make_shared<SectorRetryComputeProofEvent>());
  }

  outcome::result<void> SealingImpl::handleCommitFail(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());

    // TODO: check precommit

    // TODO: check commit

    // TODO: Check sector files

    // TODO: wait some time

    return fsm_->send(info, std::make_shared<SectorRetryComputeProofEvent>());
  }

  outcome::result<void> SealingImpl::handleFinalizeFail(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: Check sector files

    // TODO: wait some type

    return fsm_->send(info, std::make_shared<SectorRetryFinalizeEvent>());
  }

  outcome::result<void> SealingImpl::handleFaultReported(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: Implement me

    return fsm_->send(info, std::make_shared<SectorFaultedFinalEvent>());
  }

  outcome::result<void> SealingImpl::handleRemoving(
      const std::shared_ptr<SectorInfo> &info) {
    auto maybe_error = sealer_->remove(minerSector(info->sector_number));
    if (maybe_error.has_error()) {
      logger_->error(maybe_error.error().message());
      return fsm_->send(info, std::make_shared<SectorRemoveFailedEvent>());
    }

    return fsm_->send(info, std::make_shared<SectorRemovedEvent>());
  }

  outcome::result<SealingImpl::TicketInfo> SealingImpl::getTicket(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());

    OUTCOME_TRY(tipset_key, head.makeKey());

    ChainEpoch ticket_epoch =
        head.height - vm::actor::builtin::miner::kChainFinalityish;

    // TODO: get precommit info from api

    // TODO: Marshal param

    OUTCOME_TRY(randomness,
                api_->ChainGetRandomnessFromTickets(
                    tipset_key,
                    api::DomainSeparationTag::SealRandomness,
                    ticket_epoch,
                    {}));

    return TicketInfo{
        .ticket = randomness,
        .epoch = ticket_epoch,
    };
  }
}  // namespace fc::mining
