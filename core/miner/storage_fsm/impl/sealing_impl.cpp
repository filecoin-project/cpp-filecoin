/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/sealing_impl.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"

#define FSM_SEND(info, event) OUTCOME_EXCEPT(fsm_->send(info, event))

#define CALLBACK_ACTION(_action)                      \
  [this](auto info, auto event, auto from, auto to) { \
    logger_->debug("Storage FSM " #_action);          \
    _action(info, event, from, to);                   \
  }

namespace fc::mining {

  uint64_t countTrailingZeros(uint64_t n) {
    unsigned int count = 0;
    while ((n & 1) == 0) {
      count += 1;
      n >>= 1;
    }
    return count;
  }

  uint64_t countSetBits(uint64_t n) {
    unsigned int count = 0;
    while (n) {
      count += n & 1;
      n >>= 1;
    }
    return count;
  }

  std::vector<UnpaddedPieceSize> filler(UnpaddedPieceSize in) {
    uint64_t to_fill = in.padded();

    uint64_t pieces_size = countSetBits(to_fill);

    std::vector<UnpaddedPieceSize> out;
    for (size_t i = 0; i < pieces_size; ++i) {
      uint64_t next = countTrailingZeros(to_fill);
      uint64_t piece_size = uint64_t(1) << next;

      to_fill ^= piece_size;

      out.push_back(PaddedPieceSize(piece_size).unpadded());
    }
    return out;
  }

  std::vector<SealingTransition> SealingImpl::makeFSMTransitions() {
    return {
        // Main pipeline
        SealingTransition(SealingEvent::kIncoming)
            .from(SealingState::kStateUnknown)
            .to(SealingState::kPacking)
            .action(CALLBACK_ACTION(onIncoming)),
        SealingTransition(SealingEvent::kPreCommit1)
            .fromMany(SealingState::kPacking,
                      SealingState::kSealPreCommit1Fail,
                      SealingState::kSealPreCommit2Fail)
            .to(SealingState::kPreCommit1)
            .action(CALLBACK_ACTION(onPreCommit1)),
        SealingTransition(SealingEvent::kPreCommit2)
            .fromMany(SealingState::kPreCommit1,
                      SealingState::kSealPreCommit2Fail)
            .to(SealingState::kPreCommit2)
            .action(CALLBACK_ACTION(onPreCommit2)),
        SealingTransition(SealingEvent::kPreCommit)
            .fromMany(SealingState::kPreCommit2, SealingState::kPreCommitFail)
            .to(SealingState::kPreCommitting)
            .action(CALLBACK_ACTION(onPreCommit)),
        SealingTransition(SealingEvent::kPreCommitWait)
            .from(SealingState::kPreCommitting)
            .to(SealingState::kPreCommittingWait)
            .action(CALLBACK_ACTION(onPreCommitWaiting)),
        SealingTransition(SealingEvent::kWaitSeed)
            .fromMany(SealingState::kPreCommitting,
                      SealingState::kPreCommittingWait,
                      SealingState::kCommitFail)
            .to(SealingState::kWaitSeed)
            .action(CALLBACK_ACTION(onWaitSeed)),
        SealingTransition(SealingEvent::kCommit)
            .fromMany(SealingState::kWaitSeed,
                      SealingState::kCommitFail,
                      SealingState::kComputeProofFail)
            .to(SealingState::kCommitting)
            .action(CALLBACK_ACTION(onCommit)),
        SealingTransition(SealingEvent::kCommitWait)
            .from(SealingState::kCommitting)
            .to(SealingState::kCommitWait),
        SealingTransition(SealingEvent::kFinalizeSector)
            .fromMany(SealingState::kCommitWait, SealingState::kFinalizeFail)
            .to(SealingState::kFinalizeSector),
        SealingTransition(SealingEvent::kProve)
            .from(SealingState::kFinalizeSector)
            .to(SealingState::kProving),

        // Errors
        SealingTransition(SealingEvent::kUnrecoverableFailed)
            .fromMany(SealingState::kPacking,
                      SealingState::kPreCommit1,
                      SealingState::kPreCommit2,
                      SealingState::kWaitSeed,
                      SealingState::kCommitting,
                      SealingState::kCommitWait,
                      SealingState::kProving)
            .to(SealingState::kFailUnrecoverable),
        SealingTransition(SealingEvent::kSealPreCommit1Failed)
            .fromMany(SealingState::kPreCommit1,
                      SealingState::kPreCommitting,
                      SealingState::kCommitFail)
            .to(SealingState::kSealPreCommit1Fail),
        SealingTransition(SealingEvent::kSealPreCommit2Failed)
            .from(SealingState::kPreCommit2)
            .to(SealingState::kSealPreCommit2Fail),
        SealingTransition(SealingEvent::kPreCommitFailed)
            .fromMany(SealingState::kPreCommitting,
                      SealingState::kPreCommittingWait,
                      SealingState::kWaitSeed)
            .to(SealingState::kPreCommitFail),
        SealingTransition(SealingEvent::kComputeProofFailed)
            .from(SealingState::kCommitting)
            .to(SealingState::kComputeProofFail),
        SealingTransition(SealingEvent::kCommitFailed)
            .fromMany(SealingState::kCommitting, SealingState::kCommitWait)
            .to(SealingState::kCommitFail),
        SealingTransition(SealingEvent::kFinalizeFailed)
            .from(SealingState::kFinalizeSector)
            .to(SealingState::kFinalizeFail),
    };
  }

  void SealingImpl::onIncoming(const std::shared_ptr<SectorInfo> &info,
                               SealingEvent event,
                               SealingState from,
                               SealingState to) {
    logger_->info("Performing filling up rest of the sector",
                  info->sector_number);

    UnpaddedPieceSize allocated(0);
    for (const auto &piece : info->pieces) {
      allocated += piece.size.unpadded();
    }

    auto ubytes = PaddedPieceSize(sealer_->getSectorSize()).unpadded();

    if (allocated > ubytes) {
      logger_->error("too much data in sector: {} > {}", allocated, ubytes);
      return;
    }

    auto filler_sizes = filler(UnpaddedPieceSize(ubytes - allocated));

    if (!filler_sizes.empty()) {
      logger_->warn("Creating {} filler pieces for sector {}",
                    filler_sizes.size(),
                    info->sector_number);
    }

    auto maybe_result = pledgeSector(minerSector(info->sector_number),
                                     info->existingPieceSizes(),
                                     filler_sizes);

    if (maybe_result.has_error()) {
      logger_->error("Pledge sector error: {}", maybe_result.error().message());
      return;
    }

    for (const auto &new_piece : maybe_result.value()) {
      info->pieces.push_back(new_piece);
    }

    OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kPreCommit1))
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

  void SealingImpl::onPreCommit1(const std::shared_ptr<SectorInfo> &info,
                                 SealingEvent event,
                                 SealingState from,
                                 SealingState to) {
    // TODO: check Packing

    logger_->info("Performing {} sector replication", info->sector_number);

    auto maybe_ticket = getTicket(info);
    if (maybe_ticket.has_error()) {
      logger_->error("Get ticket error: {}", maybe_ticket.error().message());
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kSealPreCommit1Failed))
      return;
    }

    // TODO: add check priority
    auto maybe_result =
        sealer_->sealPreCommit1(minerSector(info->sector_number),
                                maybe_ticket.value().ticket,
                                info->pieces);

    if (maybe_result.has_error()) {
      logger_->error("Seal pre commit 1 error: {}",
                     maybe_result.error().message());
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kSealPreCommit1Failed))
      return;
    }

    info->precommit1_output = maybe_result.value();
    info->ticket = maybe_ticket.value().ticket;
    info->ticket_epoch = maybe_ticket.value().epoch;
    info->precommit2_fails = 0;

    OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kPreCommit2))
  }

  void SealingImpl::onPreCommit2(const std::shared_ptr<SectorInfo> &info,
                                 SealingEvent event,
                                 SealingState from,
                                 SealingState to) {
    // TODO: add check priority
    auto maybe_cid = sealer_->sealPreCommit2(minerSector(info->sector_number),
                                             info->precommit1_output);
    if (maybe_cid.has_error()) {
      logger_->error("Seal pre commit 2 error: {}",
                     maybe_cid.error().message());
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kSealPreCommit2Failed))
      return;
    }

    info->comm_d = maybe_cid.value().unsealed_cid;
    info->comm_r = maybe_cid.value().sealed_cid;

    OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kPreCommit))
  }

  void SealingImpl::onPreCommit(const std::shared_ptr<SectorInfo> &info,
                                SealingEvent event,
                                SealingState from,
                                SealingState to) {
    auto maybe_head = api_->ChainHead();
    if (maybe_head.has_error()) {
      logger_->error("handlePreCommitting: api error, not proceeding: {}",
                     maybe_head.error().message());
      return;
    }

    auto maybe_worker_addr = api_->StateMinerWorkerAddress(
        miner_address_, maybe_head.value().tipset);
    if (maybe_worker_addr.has_error()) {
      logger_->error("handlePreCommitting: api error, not proceeding: {}",
                     maybe_worker_addr.error().message());
      return;
    }

    // TODO: check Precommit

    // TODO: check Policy

    // TODO: CBOR params

    logger_->info("submitting precommit for sector: {}", info->sector_number);

    auto maybe_cid =
        api_->SendMsg(maybe_worker_addr.value(),
                      miner_address_,
                      vm::actor::builtin::miner::PreCommitSector::Number,
                      0,
                      1,
                      1000000,
                      {});

    if (maybe_cid.has_error()) {
      logger_->error("pushing message to mpool: {}",
                     maybe_cid.error().message());
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kPreCommitFailed))
      return;
    }

    info->precommit_message = maybe_cid.value();

    OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kPreCommitWait))
  }

  void SealingImpl::onPreCommitWaiting(const std::shared_ptr<SectorInfo> &info,
                                       SealingEvent event,
                                       SealingState from,
                                       SealingState to) {
    logger_->info("Sector precommitted: {}", info->sector_number);
    auto maybe_lookup = api_->StateWaitMsg(info->precommit_message);
    if (maybe_lookup.has_error()) {
      logger_->error("sector precommit failed: {}",
                     maybe_lookup.error().message());
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kPreCommitFailed))
      return;
    }

    if (maybe_lookup.value().receipt.exit_code != VMExitCode::kOk) {
      logger_->error("sector precommit failed: exit code is {}",
                     maybe_lookup.value().receipt.exit_code);
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kPreCommitFailed))
      return;
    }

    info->precommit_tipset = maybe_lookup.value().tipset_token;

    OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kWaitSeed))
  }

  void SealingImpl::onWaitSeed(const std::shared_ptr<SectorInfo> &info,
                               SealingEvent event,
                               SealingState from,
                               SealingState to) {
    auto maybe_precommit_info = api_->StateSectorPreCommitInfo(
        miner_address_, info->sector_number, info->precommit_tipset);
    if (maybe_precommit_info.has_error()) {
      logger_->error("getting precommit info error: {}",
                     maybe_precommit_info.error().message());
      return;
    }

    auto random_height = maybe_precommit_info.value().precommit_epoch
                         + vm::actor::builtin::miner::kPreCommitChallengeDelay;

    auto maybe_error = events_->chainAt(
        [&](const TipsetToken &token,
            ChainEpoch current_height) -> outcome::result<void> {
          // TODO: CBOR miner address
          auto maybe_randomness = api_->ChainGetRandomness(
              token,
              DomainSeparationTag::InteractiveSealChallengeSeed,
              random_height,
              {});
          if (maybe_randomness.has_error()) {
            logger_->error(
                "failed to get randomness for computing seal proof (curHeight "
                "{}; randHeight {}; tipset {}): {}",
                current_height,
                random_height,
                token,
                maybe_randomness.error().message());
            OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kPreCommitFailed))
            return maybe_randomness.error();
          }

          info->seed = maybe_randomness.value();
          info->seed_epoch = random_height;

          OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kCommit))
          return outcome::success();
        },
        [&](const TipsetToken &token) -> outcome::result<void> {
          logger_->warn("revert in interactive commit sector step");
          // TODO: cancel running and restart
          return outcome::success();
        },
        kInteractivePoRepConfidence,
        random_height);

    if (maybe_error.has_error()) {
      logger_->warn("waitForPreCommitMessage ChainAt errored: {}",
                    maybe_error.error().message());
    }
  }

  void SealingImpl::onCommit(const std::shared_ptr<SectorInfo> &info,
                             SealingEvent event,
                             SealingState from,
                             SealingState to) {
    logger_->info("scheduling seal proof computation...");

    logger_->info(
        "commit {} sector; ticket(epoch): {}({});"
        "seed(epoch): {}({}); ticket(epoch): {}({}); comm_r: {}; comm_d: {}",
        info->sector_number,
        info->ticket,
        info->ticket_epoch,
        info->seed,
        info->seed_epoch,
        info->comm_r,
        info->comm_d);

    sector_storage::SectorCids cids{
        .sealed_cid = info->comm_r,
        .unsealed_cid = info->comm_d,
    };

    // TODO: add check priority
    auto maybe_commit_1_output =
        sealer_->sealCommit1(minerSector(info->sector_number),
                             info->ticket,
                             info->seed,
                             info->pieces,
                             cids);
    if (maybe_commit_1_output.has_error()) {
      logger_->error("computing seal proof failed(1): {}",
                     maybe_commit_1_output.error().message());
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kComputeProofFailed));
      return;
    }

    auto maybe_proof = sealer_->sealCommit2(minerSector(info->sector_number),
                                            maybe_commit_1_output.value());
    if (maybe_proof.has_error()) {
      logger_->error("computing seal proof failed(2): {}",
                     maybe_proof.error().message());
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kComputeProofFailed));
      return;
    }

    auto maybe_head = api_->ChainHead();
    if (maybe_head.has_error()) {
      logger_->error("handleCommitting: api error, not proceeding: {}",
                     maybe_head.error().message());
      return;
    }

    // TODO: check Commit

    // TODO: maybe split into 2 states here

    // TODO: CBOR params

    auto maybe_worker_addr = api_->StateMinerWorkerAddress(
        miner_address_, maybe_head.value().tipset);
    if (maybe_worker_addr.has_error()) {
      logger_->error("handleCommitting: api error, not proceeding: {}",
                     maybe_worker_addr.error().message());
      return;
    }

    auto maybe_collateral = api_->StateMinerInitialPledgeCollateral(
        miner_address_, info->sector_number, maybe_head.value().tipset);
    if (maybe_collateral.has_error()) {
      logger_->error("getting initial pledge collateral: {}",
                     maybe_collateral.error().message());
      return;
    }

    auto maybe_message_cid =
        api_->SendMsg(maybe_worker_addr.value(),
                      miner_address_,
                      vm::actor::builtin::miner::ProveCommitSector::Number,
                      maybe_collateral.value(),
                      1,
                      1000000,
                      {});
    if (maybe_message_cid.has_error()) {
      logger_->error("pushing message to mpool: {}",
                     maybe_message_cid.error().message());
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kCommitFailed));
      return;
    }

    info->proof = maybe_proof.value();
    info->message = maybe_message_cid.value();
    OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kCommitWait));
  }

  outcome::result<SealingImpl::TicketInfo> SealingImpl::getTicket(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: Implement me
    return outcome::success();
  }

  std::vector<UnpaddedPieceSize> SectorInfo::existingPieceSizes() const {
    std::vector<UnpaddedPieceSize> result;

    for (const auto &piece : pieces) {
      result.push_back(piece.size.unpadded());
    }

    return result;
  }
}  // namespace fc::mining
