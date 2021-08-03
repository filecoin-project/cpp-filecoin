/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/sealing_impl.hpp"

#include <libp2p/basic/scheduler.hpp>
#include <thread>
#include <unordered_set>
#include <utility>

#include "common/bitsutil.hpp"
#include "const.hpp"
#include "miner/storage_fsm/impl/checks.hpp"
#include "miner/storage_fsm/impl/deal_info_manager_impl.hpp"
#include "miner/storage_fsm/impl/sector_stat_impl.hpp"
#include "sector_storage/zerocomm/zerocomm.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

#define WAIT(cb)                                                         \
  logger_->info("sector {}: wait before retrying", info->sector_number); \
  scheduler_->schedule(cb, getWaitingTime());

#define FSM_SEND_CONTEXT(info, event, context) \
  OUTCOME_TRY(fsm_->send(info, event, context))

#define FSM_SEND(info, event) FSM_SEND_CONTEXT(info, event, {})

#define CALLBACK_ACTION                                         \
  [](auto info, auto event, auto context, auto from, auto to) { \
    context->apply(info);                                       \
  }

namespace fc::mining {
  using api::SectorSize;
  using checks::ChecksError;
  using types::kDealSectorPriority;
  using types::Piece;
  using vm::actor::MethodParams;
  using vm::actor::builtin::types::miner::kMinSectorExpiration;
  using vm::actor::builtin::v0::miner::ProveCommitSector;

  std::chrono::milliseconds getWaitingTime(uint64_t errors_count = 0) {
    // TODO: Exponential backoff when we see consecutive failures

    return std::chrono::milliseconds(60000);  // 1 minute
  }

  SealingImpl::SealingImpl(std::shared_ptr<FullNodeApi> api,
                           std::shared_ptr<Events> events,
                           Address miner_address,
                           std::shared_ptr<Counter> counter,
                           std::shared_ptr<BufferMap> fsm_kv,
                           std::shared_ptr<Manager> sealer,
                           std::shared_ptr<PreCommitPolicy> policy,
                           std::shared_ptr<boost::asio::io_context> context,
                           std::shared_ptr<Scheduler> scheduler,
                           std::shared_ptr<PreCommitBatcher> precommit_batcher,
                           Config config)
      : scheduler_{std::move(scheduler)},
        api_(std::move(api)),
        events_(std::move(events)),
        policy_(std::move(policy)),
        counter_(std::move(counter)),
        fsm_kv_{std::move(fsm_kv)},
        miner_address_(miner_address),
        sealer_(std::move(sealer)),
        precommit_batcher_(std::move(precommit_batcher)),
        config_(config) {
    fsm_ = std::make_shared<StorageFSM>(makeFSMTransitions(), *context, true);
    fsm_->setAnyChangeAction(
        [this](auto info, auto event, auto context, auto from, auto to) {
          callbackHandle(info, event, context, from, to);
        });
    stat_ = std::make_shared<SectorStatImpl>();
    logger_ = common::createLogger("sealing");
  }

  uint64_t getDealPerSectorLimit(SectorSize size) {
    if (size < (uint64_t(64) << 30)) {
      return 256;
    }
    return 512;
  }

  outcome::result<std::shared_ptr<SealingImpl>> SealingImpl::newSealing(
      std::shared_ptr<FullNodeApi> api,
      std::shared_ptr<Events> events,
      const Address &miner_address,
      std::shared_ptr<Counter> counter,
      std::shared_ptr<BufferMap> fsm_kv,
      std::shared_ptr<Manager> sealer,
      std::shared_ptr<PreCommitPolicy> policy,
      std::shared_ptr<boost::asio::io_context> context,
      std::shared_ptr<Scheduler> scheduler,
      std::shared_ptr<PreCommitBatcher> precommit_batcher,
      Config config) {
    struct make_unique_enabler : public SealingImpl {
      make_unique_enabler(std::shared_ptr<FullNodeApi> api,
                          std::shared_ptr<Events> events,
                          Address miner_address,
                          std::shared_ptr<Counter> counter,
                          std::shared_ptr<BufferMap> fsm_kv,
                          std::shared_ptr<Manager> sealer,
                          std::shared_ptr<PreCommitPolicy> policy,
                          std::shared_ptr<boost::asio::io_context> context,
                          std::shared_ptr<Scheduler> scheduler,
                          std::shared_ptr<PreCommitBatcher> precommit_bathcer,
                          Config config)
          : SealingImpl{std::move(api),
                        std::move(events),
                        miner_address,
                        std::move(counter),
                        std::move(fsm_kv),
                        std::move(sealer),
                        std::move(policy),
                        std::move(context),
                        std::move(scheduler),
                        std::move(precommit_bathcer),
                        std::move(config)} {};
    };
    std::shared_ptr<SealingImpl> sealing =
        std::make_shared<make_unique_enabler>(api,
                                              events,
                                              miner_address,
                                              counter,
                                              fsm_kv,
                                              sealer,
                                              policy,
                                              context,
                                              scheduler,
                                              precommit_batcher,
                                              config);
    OUTCOME_TRY(sealing->fsmLoad());
    if (config.wait_deals_delay != std::chrono::milliseconds::zero()) {
      std::lock_guard lock(sealing->sectors_mutex_);
      for (const auto &sector : sealing->sectors_) {
        OUTCOME_TRY(state, sealing->fsm_->get(sector.second));
        if (state == SealingState::kWaitDeals) {
          sealing->scheduler_->schedule(
              [self{sealing}, sector_id = sector.second->sector_number]() {
                auto maybe_error = self->startPacking(sector_id);
                if (maybe_error.has_error()) {
                  self->logger_->error("starting sector {}: {}",
                                       sector_id,
                                       maybe_error.error().message());
                }
              },
              config.wait_deals_delay);
        }
      }

      // TODO: Grab on-chain sector set and diff with sectors_
    }
    return sealing;
  }

  SealingImpl::~SealingImpl() {
    logger_->info("Sealing is stopped");
    fsm_->stop();
  }

  outcome::result<void> SealingImpl::fsmLoad() {
    if (auto it{fsm_kv_->cursor()}) {
      for (it->seekToFirst(); it->isValid(); it->next()) {
        OUTCOME_TRY(_info, codec::cbor::decode<SectorInfo>(it->value()));
        auto info{std::make_shared<SectorInfo>(std::move(_info))};
        sectors_.emplace(info->sector_number, info);
        OUTCOME_TRY(fsm_->begin(info, info->state));
        callbackHandle(info, {}, {}, {}, info->state);
      }
    }
    return outcome::success();
  }

  void SealingImpl::fsmSave(const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_EXCEPT(fsm_kv_->put(
        Buffer{common::span::cbytes(std::to_string(info->sector_number))},
        codec::cbor::encode(*info).value()));
  }

  outcome::result<PieceAttributes> SealingImpl::addPieceToAnySector(
      UnpaddedPieceSize size, PieceData piece_data, DealInfo deal) {
    if (not deal.publish_cid.has_value()) {
      return SealingError::kNotPublishedDeal;
    }

    OUTCOME_TRY(cid_str, deal.publish_cid.get().toString());

    logger_->info("Adding piece (size = {}) for deal {} (publish msg: {})",
                  size,
                  deal.deal_id,
                  cid_str);

    if (primitives::piece::paddedSize(size) != size) {
      return SealingError::kCannotAllocatePiece;
    }

    OUTCOME_TRY(seal_proof_type, getCurrentSealProof());
    OUTCOME_TRY(sector_size, getSectorSize(seal_proof_type));

    if (size > PaddedPieceSize(sector_size).unpadded()) {
      return SealingError::kPieceNotFit;
    }

    bool is_start_packing = false;
    PieceAttributes piece;
    piece.size = size;

    {
      std::unique_lock lock(unsealed_mutex_);
      OUTCOME_TRY(sector_and_padding, getSectorAndPadding(size));

      piece.sector = sector_and_padding.sector;

      for (const auto &pad : sector_and_padding.pads) {
        OUTCOME_TRY(addPiece(sector_and_padding.sector,
                             pad.unpadded(),
                             PieceData("/dev/zero"),
                             boost::none));
      }

      piece.offset = unsealed_sectors_[sector_and_padding.sector].stored;

      OUTCOME_TRY(addPiece(
          sector_and_padding.sector, size, std::move(piece_data), deal));

      is_start_packing =
          unsealed_sectors_[sector_and_padding.sector].deals_number
              >= getDealPerSectorLimit(sector_size)
          || (SectorSize)piece.offset + (SectorSize)piece.size.padded()
                 == sector_size;
    }

    if (is_start_packing) {
      OUTCOME_TRY(startPacking(piece.sector));
    }

    return piece;
  }

  outcome::result<void> SealingImpl::remove(SectorNumber sector_id) {
    OUTCOME_TRY(info, getSectorInfo(sector_id));

    FSM_SEND(info, SealingEvent::kSectorRemove);

    return outcome::success();
  }

  Address SealingImpl::getAddress() const {
    return miner_address_;
  }

  std::vector<std::shared_ptr<const SectorInfo>> SealingImpl::getListSectors()
      const {
    std::lock_guard lock(sectors_mutex_);
    std::vector<std::shared_ptr<const SectorInfo>> values = {};
    for (const auto &[key, value] : sectors_) {
      values.push_back(value);
    }
    return values;
  }

  outcome::result<std::shared_ptr<SectorInfo>> SealingImpl::getSectorInfo(
      SectorNumber id) const {
    std::lock_guard lock(sectors_mutex_);
    const auto &maybe_sector{sectors_.find(id)};
    if (maybe_sector == sectors_.cend()) {
      return SealingError::kCannotFindSector;
    }
    return maybe_sector->second;
  }

  outcome::result<void> SealingImpl::forceSectorState(SectorNumber id,
                                                      SealingState state) {
    OUTCOME_TRY(info, getSectorInfo(id));
    std::shared_ptr<SectorForceContext> context =
        std::make_shared<SectorForceContext>();
    context->state = state;

    FSM_SEND_CONTEXT(info, SealingEvent::kSectorForce, context);

    return outcome::success();
  }

  outcome::result<void> SealingImpl::markForUpgrade(SectorNumber id) {
    std::unique_lock lock(upgrade_mutex_);

    if (to_upgrade_.find(id) != to_upgrade_.end()) {
      return SealingError::kAlreadyUpgradeMarked;
    }

    OUTCOME_TRY(sector_info, getSectorInfo(id));

    if (sector_info->state != SealingState::kProving) {
      return SealingError::kNotProvingState;
    }

    if (sector_info->pieces.size() != 1) {
      return SealingError::kUpgradeSeveralPieces;
    }

    if (sector_info->pieces[0].deal_info.has_value()) {
      return SealingError::kUpgradeWithDeal;
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
    if (config_.max_sealing_sectors > 0
        && stat_->currentSealing() > config_.max_sealing_sectors) {
      return outcome::success();  // cur sealing
    }

    OUTCOME_TRY(seal_proof_type, getCurrentSealProof());
    OUTCOME_TRY(sector_size, getSectorSize(seal_proof_type));

    scheduler_->schedule([self{shared_from_this()}, sector_size] {
      const UnpaddedPieceSize size = PaddedPieceSize(sector_size).unpadded();

      const auto maybe_sid = self->counter_->next();
      if (maybe_sid.has_error()) {
        self->logger_->error(maybe_sid.error().message());
        return;
      }
      auto &sid{maybe_sid.value()};

      std::vector<UnpaddedPieceSize> sizes = {size};
      const auto maybe_pieces =
          self->pledgeSector(self->minerSectorId(sid), {}, sizes);
      if (maybe_pieces.has_error()) {
        self->logger_->error(maybe_pieces.error().message());
        return;
      }

      std::vector<Piece> pieces;
      for (const auto &piece : maybe_pieces.value()) {
        pieces.push_back(Piece{
            .piece = std::move(piece),
            .deal_info = boost::none,
        });
      }

      const auto maybe_error = self->newSectorWithPieces(sid, pieces);
      if (maybe_error.has_error()) {
        self->logger_->error(maybe_error.error().message());
      }
    });

    return outcome::success();
  }

  outcome::result<void> SealingImpl::newSectorWithPieces(
      SectorNumber sector_id, std::vector<Piece> &pieces) {
    logger_->info("Creating sector with pieces {}", sector_id);
    const auto sector = std::make_shared<SectorInfo>();
    OUTCOME_TRY(fsm_->begin(sector, SealingState::kStateUnknown));
    {
      std::lock_guard lock(sectors_mutex_);
      sectors_[sector_id] = sector;
    }
    std::shared_ptr<SectorStartWithPiecesContext> context =
        std::make_shared<SectorStartWithPiecesContext>();
    context->sector_id = sector_id;
    OUTCOME_TRY(minfo, api_->StateMinerInfo(miner_address_, {}));
    context->seal_proof_type = minfo.seal_proof_type;
    context->pieces = std::move(pieces);
    FSM_SEND_CONTEXT(sector, SealingEvent::kSectorStartWithPieces, context);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::startPacking(SectorNumber id) {
    logger_->info("Start packing sector {}", id);
    OUTCOME_TRY(sector_info, getSectorInfo(id));

    FSM_SEND(sector_info, SealingEvent::kSectorStartPacking);

    {
      std::lock_guard lock(unsealed_mutex_);
      unsealed_sectors_.erase(id);
    }

    return outcome::success();
  }

  outcome::result<SealingImpl::SectorPaddingResponse>
  SealingImpl::getSectorAndPadding(UnpaddedPieceSize size) {
    OUTCOME_TRY(seal_proof_type, getCurrentSealProof());
    OUTCOME_TRY(sector_size, getSectorSize(seal_proof_type));

    for (const auto &[key, value] : unsealed_sectors_) {
      const auto pads = proofs::getRequiredPadding(value.stored, size.padded());
      if (value.stored + size.padded() + pads.size <= sector_size) {
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
      PieceData piece,
      const boost::optional<DealInfo> &deal) {
    logger_->info("Add piece to sector {}", sector_id);
    OUTCOME_TRY(seal_proof_type, getCurrentSealProof());
    OUTCOME_TRY(piece_info,
                sealer_->addPieceSync(minerSector(seal_proof_type, sector_id),
                                      unsealed_sectors_[sector_id].piece_sizes,
                                      size,
                                      std::move(piece),
                                      kDealSectorPriority));

    Piece new_piece{
        .piece = piece_info,
        .deal_info = deal,
    };

    OUTCOME_TRY(info, getSectorInfo(sector_id));
    std::shared_ptr<SectorAddPieceContext> context =
        std::make_shared<SectorAddPieceContext>();
    context->piece = new_piece;
    FSM_SEND_CONTEXT(info, SealingEvent::kSectorAddPiece, context);

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
        return SealingError::kTooManySectors;
      }
    }

    if (config_.max_wait_deals_sectors > 0
        && unsealed_sectors_.size() >= config_.max_wait_deals_sectors) {
      // TODO: check get one before max or several every time
      for (size_t i = 0; i < 10; i++) {
        if (i) {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        uint64_t best_id = 0;
        {
          std::lock_guard lock(unsealed_mutex_);

          if (unsealed_sectors_.empty()) {
            break;
          }

          const auto first_sector{unsealed_sectors_.cbegin()};
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
        const auto maybe_error = startPacking(best_id);
        if (maybe_error.has_error()) {
          logger_->error("newDealSector StartPacking error: {}",
                         maybe_error.error().message());
        }
      }
    }

    OUTCOME_TRY(sector_id, counter_->next());

    const auto sector = std::make_shared<SectorInfo>();
    logger_->info("Creating sector {}", sector_id);
    OUTCOME_TRY(fsm_->begin(sector, SealingState::kStateUnknown));
    {
      std::lock_guard lock(sectors_mutex_);
      sectors_[sector_id] = sector;
    }
    std::shared_ptr<SectorStartContext> context =
        std::make_shared<SectorStartContext>();
    context->sector_id = sector_id;
    OUTCOME_TRY(minfo, api_->StateMinerInfo(miner_address_, {}));
    context->seal_proof_type = minfo.seal_proof_type;
    FSM_SEND_CONTEXT(sector, SealingEvent::kSectorStart, context);

    if (config_.wait_deals_delay > std::chrono::milliseconds::zero()) {
      scheduler_->schedule(
          [this, sector_id]() {
            const auto maybe_error = startPacking(sector_id);
            if (maybe_error.has_error()) {
              logger_->error("starting sector {}: {}",
                             sector_id,
                             maybe_error.error().message());
            }
          },
          config_.wait_deals_delay);
      // TODO: maybe we should save it and decline if it starts early
    }

    return sector_id;
  }

  std::vector<UnpaddedPieceSize> filler(UnpaddedPieceSize in) {
    uint64_t to_fill = in.padded();

    const uint64_t pieces_size = common::countSetBits(to_fill);

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
        SealingTransition(SealingEvent::kSectorStart)
            .from(SealingState::kStateUnknown)
            .to(SealingState::kWaitDeals)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorStartWithPieces)
            .from(SealingState::kStateUnknown)
            .to(SealingState::kPacking)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorAddPiece)
            .from(SealingState::kWaitDeals)
            .toSameState()
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorStartPacking)
            .from(SealingState::kWaitDeals)
            .to(SealingState::kPacking),
        SealingTransition(SealingEvent::kSectorPacked)
            .from(SealingState::kPacking)
            .to(SealingState::kPreCommit1)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorPreCommit1)
            .from(SealingState::kPreCommit1)
            .to(SealingState::kPreCommit2)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorSealPreCommit1Failed)
            .fromMany(SealingState::kPreCommit1, SealingState::kPreCommitting)
            .to(SealingState::kSealPreCommit1Fail)
            .action(
                [](auto info, auto event, auto context, auto from, auto to) {
                  if (from == SealingState::kCommitting) {
                    return;
                  }

                  info->invalid_proofs = 0;
                  info->precommit2_fails = 0;
                }),
        SealingTransition(SealingEvent::kSectorPreCommit2)
            .from(SealingState::kPreCommit2)
            .to(SealingState::kPreCommitting)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorSealPreCommit2Failed)
            .from(SealingState::kPreCommit2)
            .to(SealingState::kSealPreCommit2Fail)
            .action(
                [](auto info, auto event, auto context, auto from, auto to) {
                  info->invalid_proofs = 0;
                  info->precommit2_fails++;
                }),
        SealingTransition(SealingEvent::kSectorPreCommitted)
            .from(SealingState::kPreCommitting)
            .to(SealingState::kPreCommittingWait)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorChainPreCommitFailed)
            .fromMany(SealingState::kPreCommitting,
                      SealingState::kPreCommittingWait,
                      SealingState::kWaitSeed,
                      SealingState::kCommitFail)
            .to(SealingState::kPreCommitFail),
        SealingTransition(SealingEvent::kSectorPreCommitLanded)
            .fromMany(SealingState::kPreCommitting,
                      SealingState::kPreCommittingWait,
                      SealingState::kPreCommitFail)
            .to(SealingState::kWaitSeed)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorSeedReady)
            .fromMany(SealingState::kWaitSeed, SealingState::kComputeProof)
            .to(SealingState::kComputeProof)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorComputeProof)
            .from(SealingState::kComputeProof)
            .to(SealingState::kCommitting)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorCommitted)
            .from(SealingState::kCommitting)
            .to(SealingState::kCommitWait)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorComputeProofFailed)
            .from(SealingState::kComputeProof)
            .to(SealingState::kComputeProofFail),
        SealingTransition(SealingEvent::kSectorCommitFailed)
            .fromMany(SealingState::kCommitting,
                      SealingState::kCommitWait,
                      SealingState::kComputeProof)
            .to(SealingState::kCommitFail),
        SealingTransition(SealingEvent::kSectorRetryCommitWait)
            .fromMany(SealingState::kCommitting,
                      SealingState::kCommitFail,
                      SealingState::kComputeProof)
            .to(SealingState::kCommitWait),
        SealingTransition(SealingEvent::kSectorProving)
            .from(SealingState::kCommitWait)
            .to(SealingState::kFinalizeSector),
        SealingTransition(SealingEvent::kSectorFinalized)
            .from(SealingState::kFinalizeSector)
            .to(SealingState::kProving),
        SealingTransition(SealingEvent::kSectorFinalizeFailed)
            .from(SealingState::kFinalizeSector)
            .to(SealingState::kFinalizeFail),
        SealingTransition(SealingEvent::kSectorDealsExpired)
            .fromMany(SealingState::kPreCommit1,
                      SealingState::kPreCommitting,
                      SealingState::kPreCommitFail,
                      SealingState::kCommitFail)
            .to(SealingState::kDealsExpired),

        SealingTransition(SealingEvent::kSectorRetrySealPreCommit1)
            .fromMany(SealingState::kSealPreCommit1Fail,
                      SealingState::kSealPreCommit2Fail,
                      SealingState::kPreCommitFail,
                      SealingState::kComputeProofFail,
                      SealingState::kCommitFail)
            .to(SealingState::kPreCommit1),
        SealingTransition(SealingEvent::kSectorRetrySealPreCommit2)
            .from(SealingState::kSealPreCommit2Fail)
            .to(SealingState::kPreCommit2),
        SealingTransition(SealingEvent::kSectorRetryPreCommit)
            .fromMany(SealingState::kPreCommitFail, SealingState::kCommitFail)
            .to(SealingState::kPreCommitting),
        SealingTransition(SealingEvent::kSectorRetryWaitSeed)
            .fromMany(SealingState::kPreCommitFail, SealingState::kCommitFail)
            .to(SealingState::kWaitSeed),
        SealingTransition(SealingEvent::kSectorRetryComputeProof)
            .fromMany(SealingState::kComputeProofFail,
                      SealingState::kCommitFail)
            .to(SealingState::kComputeProof)
            .action(
                [](auto info, auto event, auto context, auto from, auto to) {
                  info->invalid_proofs++;
                }),
        SealingTransition(SealingEvent::kSectorRetryInvalidProof)
            .from(SealingState::kCommitFail)
            .to(SealingState::kComputeProof)
            .action(
                [](auto info, auto event, auto context, auto from, auto to) {
                  info->invalid_proofs++;
                }),
        SealingTransition(SealingEvent::kSectorRetryCommitWait)
            .from(SealingState::kCommitFail)
            .to(SealingState::kPreCommittingWait),
        SealingTransition(SealingEvent::kSectorRetryCommitting)
            .fromMany(SealingState::kCommitFail, SealingState::kCommitWait)
            .to(SealingState::kCommitting),
        SealingTransition(SealingEvent::kSectorRetryFinalize)
            .from(SealingState::kFinalizeFail)
            .to(SealingState::kFinalizeSector),
        SealingTransition(SealingEvent::kSectorInvalidDealIDs)
            .fromMany(SealingState::kPreCommit1,
                      SealingState::kPreCommitting,
                      SealingState::kPreCommitFail,
                      SealingState::kCommitFail)
            .to(SealingState::kRecoverDealIDs)
            .action(CALLBACK_ACTION),

        SealingTransition(SealingEvent::kSectorFaultReported)
            .fromMany(SealingState::kProving, SealingState::kFaulty)
            .to(SealingState::kFaultReported)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorFaulty)
            .from(SealingState::kProving)
            .to(SealingState::kFaulty),
        SealingTransition(SealingEvent::kSectorRemove)
            .fromMany(SealingState::kProving,
                      SealingState::kDealsExpired,
                      SealingState::kRecoverDealIDs)
            .to(SealingState::kRemoving),
        SealingTransition(SealingEvent::kSectorRemoved)
            .from(SealingState::kRemoving)
            .to(SealingState::kRemoved),
        SealingTransition(SealingEvent::kSectorRemoveFailed)
            .from(SealingState::kRemoving)
            .to(SealingState::kRemoveFail),
        SealingTransition(SealingEvent::kSectorForce)
            .fromAny()
            .to(SealingState::kForce),
        SealingTransition(SealingEvent::kUpdateDealIds)
            .from(SealingState::kRecoverDealIDs)
            .to(SealingState::kForce),
    };
  }

  void SealingImpl::callbackHandle(
      const std::shared_ptr<SectorInfo> &info,
      SealingEvent event,
      const std::shared_ptr<SealingEventContext> &event_context,
      SealingState from,
      SealingState to) {
    stat_->updateSector(minerSectorId(info->sector_number), to);
    info->state = to;
    fsmSave(info);

    const auto maybe_error = [&]() -> outcome::result<void> {
      switch (to) {
        case SealingState::kWaitDeals: {
          logger_->info("Waiting for deals {}", info->sector_number);
          return outcome::success();
        }
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
        case SealingState::kComputeProof:
          return handleComputeProof(info);
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
        case SealingState::kDealsExpired:
          return handleDealsExpired(info);
        case SealingState::kRecoverDealIDs:
          return handleRecoverDeal(info);

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

        case SealingState::kForce: {
          std::shared_ptr<SectorForceContext> force_context =
              std::static_pointer_cast<SectorForceContext>(event_context);
          OUTCOME_TRY(fsm_->force(info, force_context->state));
          info->state = force_context->state;
          return outcome::success();
        }
        case SealingState::kStateUnknown: {
          logger_->error("sector update with undefined state!");
          return outcome::success();
        }
        default: {
          logger_->error("Unknown state {}", to);
          return outcome::success();
        }
      }
    }();
    if (maybe_error.has_error()) {
      logger_->error("Unhandled sector error ({}): {}",
                     info->sector_number,
                     maybe_error.error().message());
    }
  }

  outcome::result<void> SealingImpl::handlePacking(
      const std::shared_ptr<SectorInfo> &info) {
    logger_->info("Performing filling up rest of the sector {}",
                  info->sector_number);

    UnpaddedPieceSize allocated(0);
    for (const auto &piece : info->pieces) {
      allocated += piece.piece.size.unpadded();
    }

    OUTCOME_TRY(seal_proof_type, getCurrentSealProof());
    OUTCOME_TRY(sector_size, getSectorSize(seal_proof_type));

    const auto ubytes = PaddedPieceSize(sector_size).unpadded();

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
                pledgeSector(minerSectorId(info->sector_number),
                             info->getExistingPieceSizes(),
                             filler_sizes));

    std::shared_ptr<SectorPackedContext> context =
        std::make_shared<SectorPackedContext>();
    context->filler_pieces = std::move(result);

    FSM_SEND_CONTEXT(info, SealingEvent::kSectorPacked, context);
    return outcome::success();
  }

  outcome::result<std::vector<PieceInfo>> SealingImpl::pledgeSector(
      SectorId sector_id,
      std::vector<UnpaddedPieceSize> existing_piece_sizes,
      gsl::span<UnpaddedPieceSize> sizes) {
    if (sizes.empty()) {
      return outcome::success();
    }

    OUTCOME_TRY(seal_proof_type, getCurrentSealProof());

    std::string existing_piece_str = "empty";
    if (!existing_piece_sizes.empty()) {
      existing_piece_str = std::to_string(existing_piece_sizes[0]);
      for (size_t i = 1; i < existing_piece_sizes.size(); ++i) {
        existing_piece_str += ", ";
        existing_piece_str += std::to_string(existing_piece_sizes[i]);
      }
    }

    logger_->info("Pledge " + primitives::sector_file::sectorName(sector_id)
                  + ", contains " + existing_piece_str);

    std::vector<PieceInfo> result;

    const SectorRef sector{
        .id = sector_id,
        .proof_type = seal_proof_type,
    };

    for (const auto &size : sizes) {
      OUTCOME_TRY(
          piece_info,
          sealer_->addPieceSync(
              sector, existing_piece_sizes, size, PieceData("/dev/zero"), 0));

      existing_piece_sizes.push_back(size);

      result.push_back(piece_info);
    }

    return std::move(result);
  }

  SectorRef SealingImpl::minerSector(RegisteredSealProof seal_proof_type,
                                     SectorNumber num) const {
    return SectorRef{
        .id = minerSectorId(num),
        .proof_type = seal_proof_type,
    };
  }

  SectorId SealingImpl::minerSectorId(SectorNumber num) const {
    return SectorId{
        .miner = miner_address_.getId(),
        .sector = num,
    };
  }

  outcome::result<void> SealingImpl::handlePreCommit1(
      const std::shared_ptr<SectorInfo> &info) {
    logger_->info("PreCommit 1 sector {}", info->sector_number);
    const auto maybe_error = checks::checkPieces(miner_address_, info, api_);
    if (maybe_error.has_error()) {
      if (maybe_error == outcome::failure(ChecksError::kInvalidDeal)) {
        logger_->error("invalid dealIDs in sector {}", info->sector_number);
        std::shared_ptr<SectorInvalidDealIDContext> context =
            std::make_shared<SectorInvalidDealIDContext>();
        context->return_state = SealingState::kPreCommit1;
        FSM_SEND_CONTEXT(info, SealingEvent::kSectorInvalidDealIDs, context);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kExpiredDeal)) {
        logger_->error("expired dealIDs in sector {}", info->sector_number);
        FSM_SEND(info, SealingEvent::kSectorDealsExpired);
        return outcome::success();
      }
      return maybe_error.error();
    }

    logger_->info("Performing {} sector replication", info->sector_number);

    const auto maybe_ticket = getTicket(info);
    if (maybe_ticket.has_error()) {
      logger_->error("Get ticket error: {}", maybe_ticket.error().message());
      FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
      return outcome::success();
    }

    const auto maybe_result = sealer_->sealPreCommit1Sync(
        minerSector(info->sector_type, info->sector_number),
        maybe_ticket.value().ticket,
        info->getPieceInfos(),
        info->sealingPriority());

    if (maybe_result.has_error()) {
      logger_->error("Seal pre commit 1 error: {}",
                     maybe_result.error().message());
      FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
      return outcome::success();
    }

    std::shared_ptr<SectorPreCommit1Context> context =
        std::make_shared<SectorPreCommit1Context>();
    context->precommit1_output = std::move(maybe_result.value());
    context->ticket = maybe_ticket.value().ticket;
    context->epoch = maybe_ticket.value().epoch;
    FSM_SEND_CONTEXT(info, SealingEvent::kSectorPreCommit1, context);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handlePreCommit2(
      const std::shared_ptr<SectorInfo> &info) {
    logger_->info("PreCommit 2 sector {}", info->sector_number);
    const auto maybe_cid = sealer_->sealPreCommit2Sync(
        minerSector(info->sector_type, info->sector_number),
        info->precommit1_output,
        info->sealingPriority());
    if (maybe_cid.has_error()) {
      logger_->error("Seal pre commit 2 error: {}",
                     maybe_cid.error().message());
      FSM_SEND(info, SealingEvent::kSectorSealPreCommit2Failed);
      return outcome::success();
    }

    std::shared_ptr<SectorPreCommit2Context> context =
        std::make_shared<SectorPreCommit2Context>();
    context->unsealed = std::move(maybe_cid.value().unsealed_cid);
    context->sealed = std::move(maybe_cid.value().sealed_cid);

    FSM_SEND_CONTEXT(info, SealingEvent::kSectorPreCommit2, context);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handlePreCommitting(
      const std::shared_ptr<SectorInfo> &info) {
    logger_->info("PreCommitting sector {}", info->sector_number);
    OUTCOME_TRY(head, api_->ChainHead());

    OUTCOME_TRY(minfo, api_->StateMinerInfo(miner_address_, head->key));

    auto maybe_error = checks::checkPrecommit(
        miner_address_, info, head->key, head->height(), api_);

    if (maybe_error.has_error()) {
      if (maybe_error == outcome::failure(ChecksError::kBadCommD)) {
        logger_->error("bad CommD error (sector {})", info->sector_number);
        FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kExpiredTicket)) {
        logger_->error("ticket expired (sector {})", info->sector_number);
        FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kBadTicketEpoch)) {
        logger_->error("bad ticket epoch (sector {})", info->sector_number);
        FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kInvalidDeal)) {
        logger_->warn("invalid dealIDs in sector {}", info->sector_number);
        std::shared_ptr<SectorInvalidDealIDContext> context =
            std::make_shared<SectorInvalidDealIDContext>();
        context->return_state = SealingState::kPreCommitting;
        FSM_SEND_CONTEXT(info, SealingEvent::kSectorInvalidDealIDs, context);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kExpiredDeal)) {
        logger_->error("expired dealIDs in sector {}", info->sector_number);
        FSM_SEND(info, SealingEvent::kSectorDealsExpired);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kPrecommitOnChain)) {
        std::shared_ptr<SectorPreCommitLandedContext> context =
            std::make_shared<SectorPreCommitLandedContext>();
        context->tipset_key = head->key;
        FSM_SEND_CONTEXT(info, SealingEvent::kSectorPreCommitLanded, context);
        return outcome::success();
      }
      return maybe_error.error();
    }

    OUTCOME_TRY(network, api_->StateNetworkVersion(head->key));
    OUTCOME_TRY(seal_duration,
                checks::getMaxProveCommitDuration(network, info));
    const auto expiration = std::max<ChainEpoch>(
        policy_->expiration(info->pieces),
        head->epoch() + seal_duration + kMinSectorExpiration + 10);

    SectorPreCommitInfo params;
    params.expiration = expiration;
    params.sector = info->sector_number;
    params.registered_proof = info->sector_type;
    params.sealed_cid = info->comm_r.get();
    params.seal_epoch = info->ticket_epoch;
    params.deal_ids = info->getDealIDs();
    params.replace_deadline = {};
    params.replace_partition = {};
    params.replace_sector = {};

    auto deposit = tryUpgradeSector(params);

    const auto maybe_params = codec::cbor::encode(params);
    if (maybe_params.has_error()) {
      logger_->error("could not serialize pre-commit sector parameters: {}",
                     maybe_params.error().message());
      FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
      return outcome::success();
    }

    OUTCOME_TRY(collateral,
                api_->StateMinerPreCommitDepositForPower(
                    miner_address_, params, head->key));

    deposit = std::max(deposit, collateral);

    logger_->info("submitting precommit for sector: {}", info->sector_number);
    OUTCOME_TRY(precommit_batcher_->addPreCommit(
        *info,
        deposit,
        params,
        [=](const outcome::result<CID> &maybe_cid) -> void {
          if (maybe_cid.has_error()) {
            if (params.replace_capacity) {
              const auto maybe_error = markForUpgrade(params.replace_sector);
              if (maybe_error.has_error()) {
                logger_->error("error re-marking sector {} as for upgrade: {}",
                               info->sector_number,
                               maybe_error.error().message());
              }
            }
            logger_->error("submitting message to precommit batcher: {}",
                           maybe_cid.error().message());
            OUTCOME_EXCEPT(fsm_->send(
                info, SealingEvent::kSectorChainPreCommitFailed, {}));
          }
          std::shared_ptr<SectorPreCommittedContext> context =
              std::make_shared<SectorPreCommittedContext>();
          context->precommit_message = maybe_cid.value();
          context->precommit_deposit = deposit;
          context->precommit_info = params;
          OUTCOME_EXCEPT(
              fsm_->send(info, SealingEvent::kSectorPreCommitted, context));
        }));
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handlePreCommitWaiting(
      const std::shared_ptr<SectorInfo> &info) {
    if (!info->precommit_message) {
      logger_->error("precommit message was nil");
      FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
      return outcome::success();
    }

    logger_->info("Sector precommitted: {}", info->sector_number);
    OUTCOME_TRY(channel,
                api_->StateWaitMsg(info->precommit_message.value(),
                                   api::kNoConfidence));

    channel.waitOwn([info, this](auto &&maybe_lookup) {
      if (maybe_lookup.has_error()) {
        logger_->error("sector precommit failed: {}",
                       maybe_lookup.error().message());
        OUTCOME_EXCEPT(
            fsm_->send(info, SealingEvent::kSectorChainPreCommitFailed, {}));
        return;
      }

      if (maybe_lookup.value().receipt.exit_code != vm::VMExitCode::kOk) {
        logger_->error("sector precommit failed: exit code is {}",
                       maybe_lookup.value().receipt.exit_code);
        OUTCOME_EXCEPT(
            fsm_->send(info, SealingEvent::kSectorChainPreCommitFailed, {}));
        return;
      }

      std::shared_ptr<SectorPreCommitLandedContext> context =
          std::make_shared<SectorPreCommitLandedContext>();
      context->tipset_key = maybe_lookup.value().tipset;

      OUTCOME_EXCEPT(
          fsm_->send(info, SealingEvent::kSectorPreCommitLanded, context));
    });
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleWaitSeed(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());
    OUTCOME_TRY(precommit_info,
                checks::getStateSectorPreCommitInfo(
                    miner_address_, info, head->key, api_));
    if (!precommit_info.has_value()) {
      logger_->error("precommit info not found on chain");
      FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
      return outcome::success();
    }

    const auto random_height =
        precommit_info->precommit_epoch
        + vm::actor::builtin::types::miner::kPreCommitChallengeDelay;

    const auto maybe_error = events_->chainAt(
        [=](const Tipset &,
            ChainEpoch current_height) -> outcome::result<void> {
          OUTCOME_TRY(head, api_->ChainHead());

          OUTCOME_TRY(miner_address_encoded,
                      codec::cbor::encode(miner_address_));

          const auto maybe_randomness = api_->ChainGetRandomnessFromBeacon(
              head->key,
              crypto::randomness::DomainSeparationTag::
                  InteractiveSealChallengeSeed,
              random_height,
              MethodParams{miner_address_encoded});
          if (maybe_randomness.has_error()) {
            FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
            return maybe_randomness.error();
          }

          std::shared_ptr<SectorSeedReadyContext> context =
              std::make_shared<SectorSeedReadyContext>();

          context->seed = maybe_randomness.value();
          context->epoch = random_height;

          FSM_SEND_CONTEXT(info, SealingEvent::kSectorSeedReady, context);
          return outcome::success();
        },
        [=](const Tipset &token) -> outcome::result<void> {
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

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleComputeProof(
      const std::shared_ptr<SectorInfo> &info) {
    if (info->message.has_value()) {
      logger_->warn(
          "sector {} entered committing state with a commit message cid",
          info->sector_number);

      OUTCOME_TRY(message, api_->StateSearchMsg(*(info->message)));

      if (message.has_value()) {
        FSM_SEND(info, SealingEvent::kSectorRetryCommitWait);
        return outcome::success();
      }
    }

    logger_->info("scheduling seal proof computation...");

    logger_->info(
        "commit {} sector; ticket(epoch): {}({});"
        "seed(epoch): {}({})",
        info->sector_number,
        info->ticket,
        info->ticket_epoch,
        info->seed,
        info->seed_epoch);

    if (!(info->comm_d && info->comm_r)) {
      logger_->error("sector had nil commR or commD");
      FSM_SEND(info, SealingEvent::kSectorCommitFailed);
      return outcome::success();
    }

    const sector_storage::SectorCids cids{
        .sealed_cid = info->comm_r.get(),
        .unsealed_cid = info->comm_d.get(),
    };

    const auto maybe_commit_1_output = sealer_->sealCommit1Sync(
        minerSector(info->sector_type, info->sector_number),
        info->ticket,
        info->seed,
        info->getPieceInfos(),
        cids,
        info->sealingPriority());
    if (maybe_commit_1_output.has_error()) {
      logger_->error("computing seal proof failed(1): {}",
                     maybe_commit_1_output.error().message());
      FSM_SEND(info, SealingEvent::kSectorComputeProofFailed);
      return outcome::success();
    }

    const auto maybe_proof = sealer_->sealCommit2Sync(
        minerSector(info->sector_type, info->sector_number),
        std::move(maybe_commit_1_output.value()),
        info->sealingPriority());
    if (maybe_proof.has_error()) {
      logger_->error("computing seal proof failed(2): {}",
                     maybe_proof.error().message());
      FSM_SEND(info, SealingEvent::kSectorComputeProofFailed);
      return outcome::success();
    }

    OUTCOME_TRY(head, api_->ChainHead());

    const auto maybe_error = checks::checkCommit(miner_address_,
                                                 info,
                                                 maybe_proof.value(),
                                                 head->key,
                                                 api_,
                                                 sealer_->getProofEngine());
    if (maybe_error.has_error()) {
      logger_->error("commit check error: {}", maybe_error.error().message());
      FSM_SEND(info, SealingEvent::kSectorComputeProofFailed);
      return outcome::success();
    }

    std::shared_ptr<SectorComputeProofContext> context =
        std::make_shared<SectorComputeProofContext>();
    context->proof = std::move(maybe_proof.value());
    FSM_SEND_CONTEXT(info, SealingEvent::kSectorComputeProof, context);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleCommitting(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());

    const auto maybe_error = checks::checkCommit(miner_address_,
                                                 info,
                                                 info->proof,
                                                 head->key,
                                                 api_,
                                                 sealer_->getProofEngine());
    if (maybe_error.has_error()) {
      logger_->error("commit check error: {}", maybe_error.error().message());
      FSM_SEND(info, SealingEvent::kSectorCommitFailed);
      return outcome::success();
    }

    const auto params = ProveCommitSector::Params{
        .sector = info->sector_number,
        .proof = info->proof,
    };

    const auto maybe_params_encoded = codec::cbor::encode(params);
    if (maybe_params_encoded.has_error()) {
      logger_->error("could not serialize commit sector parameters: {}",
                     maybe_params_encoded.error().message());
      FSM_SEND(info, SealingEvent::kSectorCommitFailed);
      return outcome::success();
    }

    OUTCOME_TRY(minfo, api_->StateMinerInfo(miner_address_, head->key));

    OUTCOME_TRY(precommit_info_opt,
                checks::getStateSectorPreCommitInfo(
                    miner_address_, info, head->key, api_));
    if (!precommit_info_opt.has_value()) {
      logger_->error("precommit info not found on chain");
      FSM_SEND(info, SealingEvent::kSectorCommitFailed);
      return outcome::success();
    }

    OUTCOME_TRY(collateral,
                api_->StateMinerInitialPledgeCollateral(
                    miner_address_, precommit_info_opt->info, head->key));

    collateral -= precommit_info_opt->precommit_deposit;
    if (collateral < 0) {
      collateral = 0;
    }

    // TODO: check seed / ticket are up to date
    const auto maybe_signed_msg = api_->MpoolPushMessage(
        vm::message::UnsignedMessage(
            miner_address_,
            minfo.worker,
            0,
            collateral,
            {},
            {},
            vm::actor::builtin::v0::miner::ProveCommitSector::Number,
            MethodParams{maybe_params_encoded.value()}),
        api::kPushNoSpec);

    if (maybe_signed_msg.has_error()) {
      logger_->error("pushing message to mpool: {}",
                     maybe_signed_msg.error().message());
      FSM_SEND(info, SealingEvent::kSectorCommitFailed);
      return outcome::success();
    }

    std::shared_ptr<SectorCommittedContext> context =
        std::make_shared<SectorCommittedContext>();
    context->message = maybe_signed_msg.value().getCid();
    FSM_SEND_CONTEXT(info, SealingEvent::kSectorCommitted, context);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleCommitWait(
      const std::shared_ptr<SectorInfo> &info) {
    if (!info->message) {
      logger_->error(
          "sector {} entered commit wait state without a message cid",
          info->sector_number);
      FSM_SEND(info, SealingEvent::kSectorCommitFailed);
      return outcome::success();
    }

    OUTCOME_TRY(channel,
                api_->StateWaitMsg(info->message.get(), api::kNoConfidence));

    channel.waitOwn([=](auto &&maybe_message_lookup) {
      if (maybe_message_lookup.has_error()) {
        logger_->error("failed to wait for porep inclusion: {}",
                       maybe_message_lookup.error().message());
        OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kSectorCommitFailed, {}));
        return;
      }

      const auto &exit_code{maybe_message_lookup.value().receipt.exit_code};
      if (exit_code != vm::VMExitCode::kOk) {
        logger_->error(
            "submitting sector proof failed with code {}, message cid: {}",
            maybe_message_lookup.value().receipt.exit_code,
            info->message.get());
        if (exit_code == vm::VMExitCode::kSysErrOutOfGas
            or exit_code == vm::VMExitCode::kErrInsufficientFunds) {
          OUTCOME_EXCEPT(
              fsm_->send(info, SealingEvent::kSectorRetryCommitting, {}));
        } else {
          OUTCOME_EXCEPT(
              fsm_->send(info, SealingEvent::kSectorCommitFailed, {}));
        }
        return;
      }

      const auto maybe_error =
          api_->StateSectorGetInfo(miner_address_,
                                   info->sector_number,
                                   maybe_message_lookup.value().tipset);

      if (maybe_error.has_error()) {
        logger_->error(
            "proof validation failed, sector not found in sector set after "
            "cron: "
            "{}",
            maybe_error.error().message());
        OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kSectorCommitFailed, {}));
        return;
      }
      if (!maybe_error.value()) {
        logger_->error(
            "proof validation failed, sector not found in sector set after "
            "cron");
        OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kSectorCommitFailed, {}));
        return;
      }

      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kSectorProving, {}));
    });

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleFinalizeSector(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: Maybe wait for some finality

    const auto maybe_error = sealer_->finalizeSectorSync(
        minerSector(info->sector_type, info->sector_number),
        info->keepUnsealedRanges(),
        info->sealingPriority());
    if (maybe_error.has_error()) {
      logger_->error("finalize sector: {}", maybe_error.error().message());
      FSM_SEND(info, SealingEvent::kSectorFinalizeFailed);
      return outcome::success();
    }

    FSM_SEND(info, SealingEvent::kSectorFinalized);
    return outcome::success();
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
    WAIT([=] {
      OUTCOME_EXCEPT(
          fsm_->send(info, SealingEvent::kSectorRetrySealPreCommit1, {}));
    });
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleSealPreCommit2Fail(
      const std::shared_ptr<SectorInfo> &info) {
    auto time = getWaitingTime(info->precommit2_fails);

    if (info->precommit2_fails > 1) {
      scheduler_->schedule(
          [=] {
            OUTCOME_EXCEPT(
                fsm_->send(info, SealingEvent::kSectorRetrySealPreCommit1, {}));
          },
          time);
      return outcome::success();
    }

    scheduler_->schedule(
        [=] {
          OUTCOME_EXCEPT(
              fsm_->send(info, SealingEvent::kSectorRetrySealPreCommit2, {}));
        },
        time);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handlePreCommitFail(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());

    const auto maybe_error = checks::checkPrecommit(
        miner_address_, info, head->key, head->height(), api_);
    if (maybe_error.has_error()) {
      if (maybe_error == outcome::failure(ChecksError::kBadCommD)) {
        logger_->error("bad CommD error: {}", maybe_error.error().message());
        FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kExpiredTicket)) {
        logger_->error("ticket expired error: {}",
                       maybe_error.error().message());
        FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kBadTicketEpoch)) {
        logger_->error("bad expired: {}", maybe_error.error().message());
        FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kPrecommitNotFound)) {
        FSM_SEND(info, SealingEvent::kSectorRetryPreCommit);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kInvalidDeal)) {
        logger_->warn("invalid dealIDs in sector {}", info->sector_number);
        std::shared_ptr<SectorInvalidDealIDContext> context =
            std::make_shared<SectorInvalidDealIDContext>();
        context->return_state = SealingState::kPreCommitFail;
        FSM_SEND_CONTEXT(info, SealingEvent::kSectorInvalidDealIDs, context);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kExpiredDeal)) {
        logger_->error("expired dealIDs in sector {}", info->sector_number);
        FSM_SEND(info, SealingEvent::kSectorDealsExpired);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kSectorAllocated)) {
        logger_->error(
            "handlePreCommitFailed: sector number already allocated, not "
            "proceeding: {}",
            maybe_error.error().message());
        return outcome::success();
      }

      if (maybe_error != outcome::failure(ChecksError::kPrecommitOnChain)) {
        return maybe_error;
      }
    }

    const auto maybe_info_opt = checks::getStateSectorPreCommitInfo(
        miner_address_, info, head->key, api_);
    if (maybe_info_opt.has_error()) {
      logger_->error("Check precommit error: {}",
                     maybe_info_opt.error().message());
    } else if (maybe_info_opt.value().has_value()) {
      if (info->precommit_message.has_value()) {
        logger_->warn(
            "sector {} is precommitted on chain, but we don't have precommit "
            "message",
            info->sector_number);
        std::shared_ptr<SectorPreCommitLandedContext> context =
            std::make_shared<SectorPreCommitLandedContext>();
        context->tipset_key = head->key;
        FSM_SEND_CONTEXT(info, SealingEvent::kSectorPreCommitLanded, context);
        return outcome::success();
      }

      if (info->comm_r.has_value()) {
        logger_->warn("Context not have CommR");
        return outcome::success();
      }
      if (maybe_info_opt.value()->info.sealed_cid != info->comm_r.get()) {
        logger_->warn(
            "sector {} is precommitted on chain, with different CommR: {} != "
            "{}",
            info->sector_number,
            maybe_info_opt.value()->info.sealed_cid,
            info->comm_r.get());
        return outcome::success();  // TODO: remove when the actor allows
                                    // re-precommit
      }

      WAIT([=] {
        OUTCOME_EXCEPT(
            fsm_->send(info, SealingEvent::kSectorRetryWaitSeed, {}));
      });
      return outcome::success();
    }

    if (info->precommit_message) {
      logger_->warn(
          "retrying precommit even though the message failed to apply");
    }

    WAIT([=] {
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kSectorRetryPreCommit, {}));
    });
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleComputeProofFail(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: Check sector files

    const auto time = getWaitingTime(info->invalid_proofs);

    if (info->invalid_proofs > 1) {
      logger_->error("consecutive compute fails");
      scheduler_->schedule(
          [=] {
            OUTCOME_EXCEPT(fsm_->send(
                info, SealingEvent::kSectorSealPreCommit1Failed, {}));
          },
          time);
      return outcome::success();
    }

    scheduler_->schedule(
        [=] {
          OUTCOME_EXCEPT(
              fsm_->send(info, SealingEvent::kSectorRetryComputeProof, {}));
        },
        time);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleCommitFail(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());

    auto maybe_error = checks::checkPrecommit(
        miner_address_, info, head->key, head->height(), api_);
    if (maybe_error.has_error()) {
      if (maybe_error == outcome::failure(ChecksError::kBadCommD)) {
        logger_->error("bad CommD error: {}", maybe_error.error().message());
        FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kExpiredTicket)) {
        logger_->error("ticket expired error: {}",
                       maybe_error.error().message());
        FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kBadTicketEpoch)) {
        logger_->error("bad expired: {}", maybe_error.error().message());
        FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kInvalidDeal)) {
        logger_->warn("invalid dealIDs in sector {}", info->sector_number);
        std::shared_ptr<SectorInvalidDealIDContext> context =
            std::make_shared<SectorInvalidDealIDContext>();
        context->return_state = SealingState::kCommitFail;
        FSM_SEND_CONTEXT(info, SealingEvent::kSectorInvalidDealIDs, context);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kExpiredDeal)) {
        logger_->error("expired dealIDs in sector {}", info->sector_number);
        FSM_SEND(info, SealingEvent::kSectorDealsExpired);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kPrecommitNotFound)) {
        logger_->error("no precommit: {}", maybe_error.error().message());
        FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
        return outcome::success();
      }

      if (maybe_error != outcome::failure(ChecksError::kPrecommitOnChain)
          && maybe_error != outcome::failure(ChecksError::kSectorAllocated)) {
        return maybe_error;
      }
    }

    if (info->message.has_value()) {
      const auto maybe_message_wait =
          api_->StateSearchMsg(info->message.value());

      if (maybe_message_wait.has_error()) {
        const auto time = getWaitingTime();

        scheduler_->schedule(
            [=] {
              OUTCOME_EXCEPT(
                  fsm_->send(info, SealingEvent::kSectorRetryCommitWait, {}));
            },
            time);
        return outcome::success();
      }

      const auto &message_wait{maybe_message_wait.value()};
      if (not message_wait.has_value()) {
        FSM_SEND(info, SealingEvent::kSectorRetryCommitWait);
      }

      const auto &exit_code{message_wait.value().receipt.exit_code};
      if (exit_code == vm::VMExitCode::kOk) {
        FSM_SEND(info, SealingEvent::kSectorRetryCommitWait);
      } else if (exit_code == vm::VMExitCode::kSysErrOutOfGas) {
        FSM_SEND(info, SealingEvent::kSectorRetryCommitting);
      }
    }

    maybe_error = checks::checkCommit(miner_address_,
                                      info,
                                      info->proof,
                                      head->key,
                                      api_,
                                      sealer_->getProofEngine());
    if (maybe_error.has_error()) {
      if (maybe_error == outcome::failure(ChecksError::kBadSeed)) {
        logger_->error("seed changed, will retry: {}",
                       maybe_error.error().message());
        FSM_SEND(info, SealingEvent::kSectorRetryWaitSeed);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kInvalidProof)) {
        if (info->invalid_proofs > 0) {
          logger_->error("consecutive invalid proofs");
          WAIT([=] {
            OUTCOME_EXCEPT(fsm_->send(
                info, SealingEvent::kSectorSealPreCommit1Failed, {}));
          });
          return outcome::success();
        }

        WAIT([=] {
          OUTCOME_EXCEPT(
              fsm_->send(info, SealingEvent::kSectorRetryInvalidProof, {}));
        });
        return outcome::success();
      }

      if (maybe_error == outcome::failure(ChecksError::kPrecommitOnChain)) {
        logger_->error("no precommit on chain, will retry: {}",
                       maybe_error.error().message());
        FSM_SEND(info, SealingEvent::kSectorRetryPreCommitWait);
        return outcome::success();
      }

      if (maybe_error == outcome::failure(ChecksError::kPrecommitNotFound)) {
        FSM_SEND(info, SealingEvent::kSectorRetryPreCommit);
        return outcome::success();
      }

      if (maybe_error == outcome::failure(ChecksError::kCommitWaitFail)) {
        WAIT([=] {
          OUTCOME_EXCEPT(
              fsm_->send(info, SealingEvent::kSectorRetryCommitWait, {}));
        });
        return outcome::success();
      }

      return maybe_error;
    }

    // TODO: Check sector files

    const auto time = getWaitingTime(info->invalid_proofs);
    scheduler_->schedule(
        [=] {
          OUTCOME_EXCEPT(
              fsm_->send(info, SealingEvent::kSectorRetryComputeProof, {}));
        },
        time);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleFinalizeFail(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO: Check sector files

    WAIT([=] {
      OUTCOME_EXCEPT(fsm_->send(info, SealingEvent::kSectorRetryFinalize, {}));
    });

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleDealsExpired(
      const std::shared_ptr<SectorInfo> &info) {
    if (not info->precommit_info.has_value()) {
      // TODO(ortyomka): [FIL-382] remove expire pieces and start PC1 again
      logger_->warn("not recoverable yet");
    }

    FSM_SEND(info, SealingEvent::kSectorRemove);

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleRecoverDeal(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());

    uint64_t padding_piece = 0;
    std::vector<uint64_t> to_fix;

    for (size_t i = 0; i < info->pieces.size(); i++) {
      const auto &piece{info->pieces[i]};
      if (not piece.deal_info.has_value()) {
        OUTCOME_TRY(expect_cid,
                    sector_storage::zerocomm::getZeroPieceCommitment(
                        piece.piece.size.unpadded()));
        if (piece.piece.cid != expect_cid) {
          OUTCOME_TRY(cid_str, piece.piece.cid.toString());
          logger_->error("sector {} piece {} had non-zero PieceCID {}",
                         info->sector_number,
                         i,
                         cid_str);
          return ERROR_TEXT("Invalid CID of non-zero piece");
        }
        padding_piece++;
        continue;
      }

      const auto maybe_proposal =
          api_->StateMarketStorageDeal(piece.deal_info->deal_id, head->key);
      if (maybe_proposal.has_error()) {
        logger_->warn("getting deal {} for piece {}: {}",
                      piece.deal_info->deal_id,
                      i,
                      maybe_proposal.error().message());
        to_fix.push_back(i);
        continue;
      }
      const auto &proposal{maybe_proposal.value().proposal};

      if (proposal.provider != miner_address_) {
        logger_->warn(
            "piece {} (of {}) of sector {} refers deal {} with wrong provider: "
            "{} != {}",
            i,
            info->pieces.size(),
            info->sector_number,
            piece.deal_info->deal_id,
            encodeToString(miner_address_),
            encodeToString(proposal.provider));
        to_fix.push_back(i);
        continue;
      }

      if (proposal.piece_cid != piece.piece.cid) {
        OUTCOME_TRY(expected_cid, proposal.piece_cid.toString());
        OUTCOME_TRY(actual_cid, piece.piece.cid.toString());
        logger_->warn(
            "piece {} (of {}) of sector {} refers deal {} with wrong PieceCID: "
            "{} != {}",
            i,
            info->pieces.size(),
            info->sector_number,
            piece.deal_info->deal_id,
            actual_cid,
            expected_cid);
        to_fix.push_back(i);
        continue;
      }
      if (proposal.piece_size != piece.piece.size) {
        logger_->warn(
            "piece {} (of {}) of sector {} refers deal {} with different size: "
            "{} != {}",
            i,
            info->pieces.size(),
            info->sector_number,
            piece.deal_info->deal_id,

            piece.piece.size,

            proposal.piece_size);
        to_fix.push_back(i);
        continue;
      }

      if (head->height() >= uint64_t(proposal.start_epoch)) {
        // TODO(ortyomka): [FIL-382] try to remove the offending pieces
        logger_->error(
            "can't fix sector deals: piece {} (of {}) of sector {} refers "
            "expired deal {} - should start at {}, head {}",
            i,
            info->pieces.size(),
            info->sector_number,
            piece.deal_info->deal_id,
            proposal.start_epoch,
            head->height());
        return ERROR_TEXT("Invalid Deal");
      }
    }

    std::unordered_set<uint64_t> failed;
    std::unordered_map<uint64_t, api::DealId> updates;
    for (const auto i : to_fix) {
      const auto &piece{info->pieces[i]};

      if (not piece.deal_info->publish_cid.has_value()) {
        // TODO(ortyomka): [FIL-382] try to remove the offending pieces
        logger_->error(
            "сan't fix sector deals: piece {} (of {}) of sector {} has nil "
            "DealInfo.PublishCid (refers to deal {})",
            i,
            info->pieces.size(),
            info->sector_number,
            piece.deal_info->deal_id);

        FSM_SEND(info, SealingEvent::kSectorRemove);
        return outcome::success();
      }

      static std::shared_ptr<DealInfoManager> deal_info =
          std::make_shared<DealInfoManagerImpl>(api_);
      const auto maybe_result =
          deal_info->getCurrentDealInfo(head->key,
                                        piece.deal_info->deal_proposal,
                                        piece.deal_info->publish_cid.value());

      if (maybe_result.has_error()) {
        failed.insert(i);
        logger_->error("getting current deal info for piece {}: {}",
                       i,
                       maybe_result.error().message());
        continue;
      }

      updates[i] = maybe_result.value().deal_id;
    }

    if (not failed.empty()) {
      if (failed.size() + padding_piece == info->pieces.size()) {
        logger_->error("removing sector {}: all deals expired or unrecoverable",
                       info->sector_number);
        FSM_SEND(info, SealingEvent::kSectorRemove);
        return outcome::success();
      }

      // TODO(ortyomka): [FIL-382] try to recover
      return ERROR_TEXT("failed to recover some deals");
    }

    std::shared_ptr<SectorUpdateDealIds> context =
        std::make_shared<SectorUpdateDealIds>();
    context->updates = std::move(updates);

    FSM_SEND_CONTEXT(info, SealingEvent::kUpdateDealIds, context);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleFaultReported(
      const std::shared_ptr<SectorInfo> &info) {
    if (!info->fault_report_message.has_value()) {
      return SealingError::kNoFaultMessage;
    }

    OUTCOME_TRY(channel,
                api_->StateWaitMsg(info->fault_report_message.get(),
                                   api::kNoConfidence));
    OUTCOME_TRY(message, channel.waitSync());

    if (message.receipt.exit_code != vm::VMExitCode::kOk) {
      logger_->error(
          " declaring sector fault failed (exit={}, msg={}) (id: {})",
          message.receipt.exit_code,
          info->fault_report_message.get(),
          info->sector_number);
      return SealingError::kFailSubmit;
    }

    FSM_SEND(info, SealingEvent::kSectorFaultedFinal);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleRemoving(
      const std::shared_ptr<SectorInfo> &info) {
    const auto maybe_error =
        sealer_->remove(minerSector(info->sector_type, info->sector_number));
    if (maybe_error.has_error()) {
      logger_->error(maybe_error.error().message());
      FSM_SEND(info, SealingEvent::kSectorRemoveFailed);
      return outcome::success();
    }

    FSM_SEND(info, SealingEvent::kSectorRemoved);
    return outcome::success();
  }

  outcome::result<SealingImpl::TicketInfo> SealingImpl::getTicket(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());

    ChainEpoch ticket_epoch =
        head->height() - vm::actor::builtin::types::miner::kChainFinality;

    OUTCOME_TRY(address_encoded, codec::cbor::encode(miner_address_));

    OUTCOME_TRY(precommit_info,
                checks::getStateSectorPreCommitInfo(
                    miner_address_, info, head->key, api_));

    if (precommit_info.has_value()) {
      ticket_epoch = precommit_info->info.seal_epoch;
    }

    OUTCOME_TRY(randomness,
                api_->ChainGetRandomnessFromTickets(
                    head->key,
                    api::DomainSeparationTag::SealRandomness,
                    ticket_epoch,
                    MethodParams{address_encoded}));

    return TicketInfo{
        .ticket = randomness,
        .epoch = ticket_epoch,
    };
  }

  TokenAmount SealingImpl::tryUpgradeSector(SectorPreCommitInfo &params) {
    if (params.deal_ids.empty()) {
      return 0;
    }

    const auto replace = maybeUpgradableSector();
    if (replace) {
      const auto maybe_location = api_->StateSectorPartition(
          miner_address_, *replace, api::TipsetKey());
      if (maybe_location.has_error()) {
        logger_->error(
            "error calling StateSectorPartition for replaced sector: {}",
            maybe_location.error().message());
        return 0;
      }

      params.replace_capacity = true;
      params.replace_sector = *replace;
      params.replace_deadline = maybe_location.value().deadline;
      params.replace_partition = maybe_location.value().partition;

      const auto maybe_replace_info =
          api_->StateSectorGetInfo(miner_address_, *replace, api::TipsetKey());
      if (maybe_replace_info.has_error()) {
        logger_->error(
            "error calling StateSectorGetInfo for replaced sector: {}",
            maybe_replace_info.error().message());
        return 0;
      }
      const auto &info{maybe_replace_info.value()};
      if (!info) {
        logger_->error("couldn't find sector info for sector to replace {}",
                       *replace);
        return 0;
      }

      params.expiration = std::min(params.expiration, info->expiration);

      return info->init_pledge;
    }

    return 0;
  }

  boost::optional<SectorNumber> SealingImpl::maybeUpgradableSector() {
    std::lock_guard lock(upgrade_mutex_);
    if (to_upgrade_.empty()) {
      return boost::none;
    }

    // TODO: checks to match actor constraints
    // Note: maybe here should be loop

    const auto result = *(to_upgrade_.begin());
    to_upgrade_.erase(to_upgrade_.begin());
    return result;
  }

  outcome::result<RegisteredSealProof> SealingImpl::getCurrentSealProof()
      const {
    OUTCOME_TRY(miner_info, api_->StateMinerInfo(miner_address_, {}));

    return miner_info.seal_proof_type;
  }

}  // namespace fc::mining

OUTCOME_CPP_DEFINE_CATEGORY(fc::mining, SealingError, e) {
  using E = fc::mining::SealingError;
  switch (e) {
    case E::kPieceNotFit:
      return "SealingError: piece cannot fit into a sector";
    case E::kCannotAllocatePiece:
      return "SealingError: cannot allocate unpadded piece";
    case E::kCannotFindSector:
      return "SealingError: sector not found";
    case E::kAlreadyUpgradeMarked:
      return "SealingError: sector already marked for upgrade";
    case E::kNotProvingState:
      return "SealingError: can't mark sectors not in the 'Proving' state for "
             "upgrade";
    case E::kUpgradeSeveralPieces:
      return "SealingError: not a committed-capacity sector, expected 1 piece";
    case E::kUpgradeWithDeal:
      return "SealingError: not a committed-capacity sector, has deals";
    case E::kTooManySectors:
      return "SealingError: too many sectors sealing";
    case E::kNoFaultMessage:
      return "SealingError: entered fault reported state without a "
             "FaultReportMsg cid";
    case E::kFailSubmit:
      return "SealingError: submitting fault declaration failed";
    case E::kSectorAllocatedError:
      return "SealingError: sectorNumber is allocated, but PreCommit info "
             "wasn't found on chain";
    case E::kNotPublishedDeal:
      return "SealingError: deal cid is none";
    default:
      return "SealingError: unknown error";
  }
}
