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
#include "miner/storage_fsm/impl/sector_stat_impl.hpp"
#include "sector_storage/zerocomm/zerocomm.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "vm/actor/builtin/types/market/deal_info_manager/impl/deal_info_manager_impl.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/builtin/v7/miner/miner_actor.hpp"

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
  using api::kPushNoSpec;
  using api::SectorSize;
  using checks::ChecksError;
  using sector_storage::ReplicaUpdateOut;
  using types::kDealSectorPriority;
  using types::Piece;
  using vm::actor::MethodParams;
  using vm::actor::builtin::types::market::kDealMinDuration;
  using vm::actor::builtin::types::market::deal_info_manager::DealInfoManager;
  using vm::actor::builtin::types::market::deal_info_manager::
      DealInfoManagerImpl;
  using vm::actor::builtin::types::miner::kChainFinality;
  using vm::actor::builtin::types::miner::kMaxPreCommitRandomnessLookback;
  using vm::actor::builtin::types::miner::kMaxProveCommitDuration;
  using vm::actor::builtin::types::miner::kMaxSectorExpirationExtension;
  using vm::actor::builtin::types::miner::kMinSectorExpiration;
  using vm::actor::builtin::types::miner::kPreCommitChallengeDelay;
  using vm::actor::builtin::v0::miner::PreCommitSector;
  using vm::actor::builtin::v0::miner::ProveCommitSector;
  using vm::actor::builtin::v7::miner::ProveReplicaUpdates;
  using vm::actor::builtin::v7::miner::ReplicaUpdate;

  std::chrono::milliseconds getWaitingTime(uint64_t errors_count = 0) {
    // TODO(ortyomka): Exponential backoff when we see consecutive failures

    return std::chrono::milliseconds(60000);  // 1 minute
  }

  outcome::result<bool> sectorActive(std::shared_ptr<FullNodeApi> api,
                                     const Address &miner_address,
                                     const TipsetKey &tipset_key,
                                     SectorNumber sector) {
    OUTCOME_TRY(active_sectors,
                api->StateMinerActiveSectors(miner_address, tipset_key));

    return find_if(active_sectors.begin(),
                   active_sectors.end(),
                   [sector_id{sector}](const auto &sector) {
                     return sector.sector == sector_id;
                   })
           != active_sectors.end();
  }

  SealingImpl::SealingImpl(
      std::shared_ptr<FullNodeApi> api,
      std::shared_ptr<Events> events,
      Address miner_address,
      std::shared_ptr<Counter> counter,
      std::shared_ptr<BufferMap> fsm_kv,
      std::shared_ptr<Manager> sealer,
      std::shared_ptr<PreCommitPolicy> policy,
      const std::shared_ptr<boost::asio::io_context> &context,
      std::shared_ptr<Scheduler> scheduler,
      std::shared_ptr<StorageFSM> fsm,
      std::shared_ptr<PreCommitBatcher> precommit_batcher,
      AddressSelector address_selector,
      std::shared_ptr<FeeConfig> fee_config,
      Config config)
      : scheduler_{std::move(scheduler)},
        fsm_{std::move(fsm)},
        context_(std::move(context)),
        api_(std::move(api)),
        events_(std::move(events)),
        policy_(std::move(policy)),
        counter_(std::move(counter)),
        fsm_kv_{std::move(fsm_kv)},
        miner_address_(std::move(miner_address)),
        fee_config_(std::move(fee_config)),
        sealer_(std::move(sealer)),
        precommit_batcher_(std::move(precommit_batcher)),
        address_selector_(std::move(address_selector)),
        config_(config) {
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
      const std::shared_ptr<FullNodeApi> &api,
      const std::shared_ptr<Events> &events,
      const Address &miner_address,
      const std::shared_ptr<Counter> &counter,
      const std::shared_ptr<BufferMap> &fsm_kv,
      const std::shared_ptr<Manager> &sealer,
      const std::shared_ptr<PreCommitPolicy> &policy,
      const std::shared_ptr<boost::asio::io_context> &context,
      const std::shared_ptr<Scheduler> &scheduler,
      const std::shared_ptr<PreCommitBatcher> &precommit_batcher,
      const AddressSelector &address_selector,
      const std::shared_ptr<FeeConfig> &fee_config,
      Config config) {
    OUTCOME_TRY(fsm,
                StorageFSM::createFsm(makeFSMTransitions(), *context, true));
    struct make_unique_enabler : public SealingImpl {
      make_unique_enabler(
          std::shared_ptr<FullNodeApi> api,
          std::shared_ptr<Events> events,
          Address miner_address,
          std::shared_ptr<Counter> counter,
          std::shared_ptr<BufferMap> fsm_kv,
          std::shared_ptr<Manager> sealer,
          std::shared_ptr<PreCommitPolicy> policy,
          const std::shared_ptr<boost::asio::io_context> &context,
          std::shared_ptr<Scheduler> scheduler,
          std::shared_ptr<StorageFSM> fsm,
          std::shared_ptr<PreCommitBatcher> precommit_bathcer,
          AddressSelector address_selector,
          std::shared_ptr<FeeConfig> fee_config,
          Config config)
          : SealingImpl{std::move(api),
                        std::move(events),
                        std::move(miner_address),
                        std::move(counter),
                        std::move(fsm_kv),
                        std::move(sealer),
                        std::move(policy),
                        context,
                        std::move(scheduler),
                        std::move(fsm),
                        std::move(precommit_bathcer),
                        std::move(address_selector),
                        std::move(fee_config),
                        config} {};
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
                                              fsm,
                                              precommit_batcher,
                                              address_selector,
                                              fee_config,
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

      // TODO(ortyomka): Grab on-chain sector set and diff with sectors_
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
        copy(common::span::cbytes(std::to_string(info->sector_number))),
        codec::cbor::encode(*info).value()));
  }

  outcome::result<PieceLocation> SealingImpl::addPieceToAnySector(
      const UnpaddedPieceSize &size,
      PieceData piece_data,
      const DealInfo &deal) {
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
    PieceLocation piece_location;

    {
      std::unique_lock lock(unsealed_mutex_);
      OUTCOME_TRY(sector_and_padding, getSectorAndPadding(size));

      auto &unsealed_sector{unsealed_sectors_[sector_and_padding.sector]};

      piece_location.sector = sector_and_padding.sector;
      piece_location.size = size.padded();

      auto context = std::make_shared<SectorAddPiecesContext>();
      context->pieces.reserve(sector_and_padding.padding.pads.size());

      auto sector_ref = minerSector(seal_proof_type, piece_location.sector);

      if (sector_and_padding.padding.size != 0) {
        OUTCOME_TRY(
            sealer_->addPieceSync(sector_ref,
                                  VectorCoW(gsl::span<const UnpaddedPieceSize>(
                                      unsealed_sector.piece_sizes)),
                                  sector_and_padding.padding.size.unpadded(),
                                  PieceData::makeNull(),
                                  kDealSectorPriority));

        unsealed_sector.stored += sector_and_padding.padding.size;

        for (const auto &pad : sector_and_padding.padding.pads) {
          OUTCOME_TRY(
              zerocomm,
              sector_storage::zerocomm::getZeroPieceCommitment(pad.unpadded()));

          context->pieces.push_back(Piece{
              .piece =
                  PieceInfo{
                      .size = pad,
                      .cid = std::move(zerocomm),
                  },
              .deal_info = boost::none,
          });

          unsealed_sector.piece_sizes.push_back(pad.unpadded());
        }
      }

      piece_location.offset = unsealed_sector.stored;

      logger_->info("Add piece to sector {}", piece_location.sector);
      OUTCOME_TRY(
          piece_info,
          sealer_->addPieceSync(sector_ref,
                                VectorCoW(gsl::span<const UnpaddedPieceSize>(
                                    unsealed_sector.piece_sizes)),
                                size,
                                std::move(piece_data),
                                kDealSectorPriority));

      context->pieces.push_back(Piece{
          .piece = piece_info,
          .deal_info = deal,
      });

      unsealed_sector.deals_number++;
      unsealed_sector.stored += piece_info.size;
      unsealed_sector.piece_sizes.push_back(piece_info.size.unpadded());

      OUTCOME_TRY(info, getSectorInfo(piece_location.sector));
      FSM_SEND_CONTEXT(info, SealingEvent::kSectorAddPieces, context);

      is_start_packing =
          unsealed_sector.deals_number >= getDealPerSectorLimit(sector_size)
          || SectorSize(piece_location.offset) + SectorSize(piece_location.size)
                 == sector_size;
    }

    if (is_start_packing) {
      OUTCOME_TRY(startPacking(piece_location.sector));
    }

    return piece_location;
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

  outcome::result<void> SealingImpl::markForSnapUpgrade(SectorNumber id) {
    // TODO(a.chernyshov)
    // https://github.com/filecoin-project/lotus/blob/362c73bfbdb8c6d5f1d110b25ee33faa2b5c8dcc/extern/storage-sealing/upgrade_queue.go#L53-L62
    // check staging - waitDealsSectors < config.maxWaitDealsSectors

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

    OUTCOME_TRY(head, api_->ChainHead());
    OUTCOME_TRY(active, sectorActive(api_, miner_address_, head->key, id));
    // Ensure the upgraded sector is active
    if (not active) {
      return SealingError::kCannotMarkInactiveSector;
    }

    OUTCOME_TRY(onchain_info,
                api_->StateSectorGetInfo(miner_address_, id, head->key));
    if (onchain_info->expiration - head->epoch() < kDealMinDuration) {
      logger_->error(
          "pointless to upgrade sector {}, expiration {} is less than a min "
          "deal duration away from current epoch. Upgrade expiration before "
          "marking for upgrade",
          id,
          onchain_info->expiration);
      return SealingError::kSectorExpirationError;
    }

    {
      std::unique_lock lock(unsealed_mutex_);
      unsealed_sectors_[id] = UnsealedSectorInfo{
          .deals_number = 0,
          .stored = PaddedPieceSize(0),
          .piece_sizes = {},
      };
    }

    logger_->info("Sector {} is marked for Snap upgrade.", id);
    FSM_SEND_CONTEXT(sector_info,
                     SealingEvent::kSectorStartCCUpdate,
                     std::make_shared<SectorStartCCUpdateContext>());

    return outcome::success();
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
      const auto &sid{maybe_sid.value()};

      const std::vector<UnpaddedPieceSize> sizes = {size};

      self->pledgeSector(
          self->minerSectorId(sid),
          {},
          sizes,
          [self,
           sid](const outcome::result<std::vector<PieceInfo>> &maybe_pieces) {
            if (maybe_pieces.has_error()) {
              self->logger_->error(maybe_pieces.error().message());
              return;
            }

            std::vector<Piece> pieces;
            for (auto &piece : maybe_pieces.value()) {
              pieces.push_back(Piece{
                  .piece = std::move(piece),
                  .deal_info = boost::none,
              });
            }

            const auto maybe_error =
                self->newSectorWithPieces(sid, std::move(pieces));
            if (maybe_error.has_error()) {
              self->logger_->error(maybe_error.error().message());
            }
          });
    });
    return outcome::success();
  }

  outcome::result<void> SealingImpl::newSectorWithPieces(
      SectorNumber sector_id, std::vector<Piece> pieces) {
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
    OUTCOME_TRYA(context->seal_proof_type, getCurrentSealProof());
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
      auto pads = proofs::getRequiredPadding(value.stored, size.padded());
      if (value.stored + size.padded() + pads.size <= sector_size) {
        return SectorPaddingResponse{
            .sector = key,
            .padding = std::move(pads),
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
        .padding = RequiredPadding{.size = PaddedPieceSize(0)},
    };
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<SectorNumber> SealingImpl::newDealSector() {
    if (config_.max_sealing_sectors_for_deals > 0) {
      if (stat_->currentSealing() > config_.max_sealing_sectors_for_deals) {
        return SealingError::kTooManySectors;
      }
    }

    if (config_.max_wait_deals_sectors > 0
        && unsealed_sectors_.size() >= config_.max_wait_deals_sectors) {
      // TODO(ortyomka): check get one before max or several every time
      for (size_t i = 0; i < 10; i++) {
        if (i != 0) {
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
    OUTCOME_TRYA(context->seal_proof_type, getCurrentSealProof());
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
      // TODO(ortyomka): maybe we should save it and decline if it starts early
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
        SealingTransition(SealingEvent::kSectorAddPieces)
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
            .fromMany(SealingState::kPreCommit1,
                      SealingState::kPreCommitting,
                      SealingState::kPreCommitFail,
                      SealingState::kCommitFail,
                      SealingState::kComputeProofFail,
                      SealingState::kSubmitPreCommitBatch)
            .to(SealingState::kSealPreCommit1Fail)
            .action(
                [](auto info, auto event, auto context, auto from, auto to) {
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
        SealingTransition(SealingEvent::kSectorBatchSend)
            .from(SealingState::kPreCommitting)
            .to(SealingState::kSubmitPreCommitBatch),
        SealingTransition(SealingEvent::kSectorPreCommitted)
            .from(SealingState::kPreCommitting)
            .to(SealingState::kPreCommittingWait)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorPreCommittedBatch)
            .from(SealingState::kSubmitPreCommitBatch)
            .to(SealingState::kPreCommittingWait)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorChainPreCommitFailed)
            .fromMany(SealingState::kPreCommitting,
                      SealingState::kSubmitPreCommitBatch,
                      SealingState::kPreCommittingWait,
                      SealingState::kWaitSeed,
                      SealingState::kCommitFail)
            .to(SealingState::kPreCommitFail),
        SealingTransition(SealingEvent::kSectorPreCommitLanded)
            .fromMany(SealingState::kPreCommitting,
                      SealingState::kSubmitPreCommitBatch,
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
            .fromMany(SealingState::kCommitting, SealingState::kCommitFail)
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
                      SealingState::kSubmitPreCommitBatch,
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
        SealingTransition(SealingEvent::kSectorRetryCommitting)
            .fromMany(SealingState::kCommitFail, SealingState::kCommitWait)
            .to(SealingState::kCommitting),
        SealingTransition(SealingEvent::kSectorRetryFinalize)
            .from(SealingState::kFinalizeFail)
            .to(SealingState::kFinalizeSector),
        SealingTransition(SealingEvent::kSectorInvalidDealIDs)
            .fromMany(SealingState::kPreCommit1,
                      SealingState::kPreCommitting,
                      SealingState::kSubmitPreCommitBatch,
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

        // Snap Deals
        SealingTransition(SealingEvent::kSectorAddPieces)
            .from(SealingState::kSnapDealsWaitDeals)
            .toSameState()
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorStartPacking)
            .from(SealingState::kSnapDealsWaitDeals)
            .to(SealingState::kSnapDealsPacking),
        SealingTransition(SealingEvent::kSectorAddPieceFailed)
            .from(SealingState::kSnapDealsAddPiece)
            .to(SealingState::kSnapDealsAddPieceFailed),

        SealingTransition(SealingEvent::kSectorPacked)
            .from(SealingState::kSnapDealsPacking)
            .to(SealingState::kUpdateReplica),

        SealingTransition(SealingEvent::kSectorReplicaUpdate)
            .from(SealingState::kUpdateReplica)
            .to(SealingState::kProveReplicaUpdate)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorUpdateReplicaFailed)
            .from(SealingState::kUpdateReplica)
            .to(SealingState::kReplicaUpdateFailed),
        SealingTransition(SealingEvent::kSectorDealsExpired)
            .from(SealingState::kUpdateReplica)
            .to(SealingState::kSnapDealsDealsExpired),
        SealingTransition(SealingEvent::kSectorInvalidDealIDs)
            .from(SealingState::kUpdateReplica)
            .to(SealingState::kSnapDealsRecoverDealIDs),

        SealingTransition(SealingEvent::kSectorProveReplicaUpdate)
            .from(SealingState::kProveReplicaUpdate)
            .to(SealingState::kSubmitReplicaUpdate)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorProveReplicaUpdateFailed)
            .from(SealingState::kProveReplicaUpdate)
            .to(SealingState::kReplicaUpdateFailed),
        SealingTransition(SealingEvent::kSectorDealsExpired)
            .from(SealingState::kProveReplicaUpdate)
            .to(SealingState::kSnapDealsDealsExpired),
        SealingTransition(SealingEvent::kSectorInvalidDealIDs)
            .from(SealingState::kProveReplicaUpdate)
            .to(SealingState::kSnapDealsRecoverDealIDs),
        SealingTransition(SealingEvent::kSectorAbortUpgrade)
            .from(SealingState::kProveReplicaUpdate)
            .to(SealingState::kAbortUpgrade),

        SealingTransition(SealingEvent::kSectorReplicaUpdateSubmitted)
            .from(SealingState::kSubmitReplicaUpdate)
            .to(SealingState::kReplicaUpdateWait)
            .action(CALLBACK_ACTION),
        SealingTransition(SealingEvent::kSectorSubmitReplicaUpdateFailed)
            .from(SealingState::kSubmitReplicaUpdate)
            .to(SealingState::kReplicaUpdateFailed),

        SealingTransition(SealingEvent::kSectorReplicaUpdateLanded)
            .from(SealingState::kReplicaUpdateWait)
            .to(SealingState::kFinalizeReplicaUpdate),
        SealingTransition(SealingEvent::kSectorSubmitReplicaUpdateFailed)
            .from(SealingState::kReplicaUpdateWait)
            .to(SealingState::kReplicaUpdateFailed),
        SealingTransition(SealingEvent::kSectorAbortUpgrade)
            .from(SealingState::kReplicaUpdateWait)
            .to(SealingState::kAbortUpgrade),

        SealingTransition(SealingEvent::kSectorFinalized)
            .from(SealingState::kFinalizeReplicaUpdate)
            .to(SealingState::kUpdateActivating),
        SealingTransition(SealingEvent::kSectorFinalizeFailed)
            .from(SealingState::kFinalizeReplicaUpdate)
            .to(SealingState::kFinalizeReplicaUpdateFailed),

        SealingTransition(SealingEvent::kSectorUpdateActive)
            .from(SealingState::kUpdateActivating)
            .to(SealingState::kReleaseSectorKey),

        SealingTransition(SealingEvent::kSectorReleaseKeyFailed)
            .from(SealingState::kReleaseSectorKey)
            .to(SealingState::kReleaseSectorKeyFailed),
        SealingTransition(SealingEvent::kSectorKeyReleased)
            .from(SealingState::kReleaseSectorKey)
            .to(SealingState::kProving),

        SealingTransition(SealingEvent::kSectorRetryWaitDeals)
            .from(SealingState::kSnapDealsAddPieceFailed)
            .to(SealingState::kSnapDealsWaitDeals),

        SealingTransition(SealingEvent::kSectorAbortUpgrade)
            .from(SealingState::kSnapDealsDealsExpired)
            .to(SealingState::kAbortUpgrade),

        SealingTransition(SealingEvent::kSectorUpdateDealIDs)
            .from(SealingState::kSnapDealsRecoverDealIDs)
            .to(SealingState::kSubmitReplicaUpdate),
        SealingTransition(SealingEvent::kSectorAbortUpgrade)
            .from(SealingState::kSnapDealsRecoverDealIDs)
            .to(SealingState::kAbortUpgrade),

        SealingTransition(SealingEvent::kSectorRetrySubmitReplicaUpdateWait)
            .from(SealingState::kReplicaUpdateFailed)
            .to(SealingState::kReplicaUpdateWait),
        SealingTransition(SealingEvent::kSectorRetrySubmitReplicaUpdate)
            .from(SealingState::kReplicaUpdateFailed)
            .to(SealingState::kSubmitReplicaUpdate),
        SealingTransition(SealingEvent::kSectorRetryReplicaUpdate)
            .from(SealingState::kReplicaUpdateFailed)
            .to(SealingState::kUpdateReplica),
        SealingTransition(SealingEvent::kSectorRetryProveReplicaUpdate)
            .from(SealingState::kReplicaUpdateFailed)
            .to(SealingState::kProveReplicaUpdate),
        SealingTransition(SealingEvent::kSectorInvalidDealIDs)
            .from(SealingState::kReplicaUpdateFailed)
            .to(SealingState::kSnapDealsRecoverDealIDs),
        SealingTransition(SealingEvent::kSectorDealsExpired)
            .from(SealingState::kReplicaUpdateFailed)
            .to(SealingState::kSnapDealsDealsExpired),

        SealingTransition(SealingEvent::kSectorUpdateActive)
            .from(SealingState::kReleaseSectorKeyFailed)
            .to(SealingState::kReleaseSectorKey),

        SealingTransition(SealingEvent::kSectorRetryFinalize)
            .from(SealingState::kFinalizeReplicaUpdateFailed)
            .to(SealingState::kFinalizeReplicaUpdate),

        SealingTransition(SealingEvent::kSectorRevertUpgradeToProving)
            .from(SealingState::kAbortUpgrade)
            .to(SealingState::kProving),

        SealingTransition(SealingEvent::kSectorStartCCUpdate)
            .from(SealingState::kProving)
            .to(SealingState::kSnapDealsWaitDeals)
            .action(CALLBACK_ACTION),
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
        case SealingState::kSubmitPreCommitBatch:
          return handleSubmitPreCommitBatch(info);
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

        case SealingState::kSnapDealsWaitDeals:
          return outcome::success();
        case SealingState::kSnapDealsPacking:
          return handlePacking(info);
        case SealingState::kUpdateReplica:
          return handleReplicaUpdate(info);
        case SealingState::kProveReplicaUpdate:
          return handleProveReplicaUpdate(info);
        case SealingState::kSubmitReplicaUpdate:
          return handleSubmitReplicaUpdate(info);
        case SealingState::kReplicaUpdateWait:
          return handleReplicaUpdateWait(info);
        case SealingState::kFinalizeReplicaUpdate:
          return handleFinalizeReplicaUpdate(info);
        case SealingState::kUpdateActivating:
          return handleUpdateActivating(info);
        case SealingState::kReleaseSectorKey:
          return handleReleaseSectorKey(info);

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

        case SealingState::kSnapDealsAddPieceFailed:
          return handleSnapDealsAddPieceFailed(info);
        case SealingState::kSnapDealsDealsExpired:
          return handleSnapDealsDealsExpired(info);
        case SealingState::kSnapDealsRecoverDealIDs:
          return handleSnapDealsRecoverDealIDs(info);
        case SealingState::kAbortUpgrade:
          return handleAbortUpgrade(info);
        case SealingState::kReplicaUpdateFailed:
          return handleReplicaUpdateFailed(info);
        case SealingState::kReleaseSectorKeyFailed:
          return handleReleaseSectorKeyFailed(info);
        case SealingState::kFinalizeReplicaUpdateFailed:
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

    pledgeSector(
        minerSectorId(info->sector_number),
        info->getExistingPieceSizes(),
        filler_sizes,
        [fsm{fsm_}, info, logger{logger_}](
            const outcome::result<std::vector<PieceInfo>> &result) {
          if (result.has_error()) {
            logger->error("handlePacking: {}", result.error().message());
            return;
          }

          std::shared_ptr<SectorPackedContext> context =
              std::make_shared<SectorPackedContext>();
          context->filler_pieces = std::move(result.value());

          OUTCOME_EXCEPT(fsm->send(info, SealingEvent::kSectorPacked, context));
        });
    return outcome::success();
  }

  void SealingImpl::pledgeSector(
      SectorId sector_id,
      std::vector<UnpaddedPieceSize> existing_piece_sizes,
      const std::vector<UnpaddedPieceSize> &sizes,
      const std::function<void(outcome::result<std::vector<PieceInfo>>)> &cb) {
    if (sizes.empty()) {
      return cb(std::vector<PieceInfo>());
    }

    OUTCOME_CB(const RegisteredSealProof seal_proof_type,
               getCurrentSealProof());

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
    result.reserve(sizes.size());

    const SectorRef sector{
        .id = sector_id,
        .proof_type = seal_proof_type,
    };

    UnpaddedPieceSize filler(0);

    for (const auto &size : sizes) {
      filler += size;

      OUTCOME_CB(CID piece_cid,
                 sector_storage::zerocomm::getZeroPieceCommitment(size))

      result.push_back(
          PieceInfo{.size = size.padded(), .cid = std::move(piece_cid)});
    }

    sealer_->addPiece(
        sector,
        VectorCoW(std::move(existing_piece_sizes)),
        filler,
        PieceData::makeNull(),
        [fill = std::move(result), cb](const auto &maybe_error) -> void {
          if (maybe_error.has_error()) {
            return cb(maybe_error.error());
          }

          return cb(fill);
        },
        0);
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

    sealer_->sealPreCommit1(
        minerSector(info->sector_type, info->sector_number),
        maybe_ticket.value().ticket,
        info->getPieceInfos(),
        [ticket{maybe_ticket.value()}, info{info}, logger{logger_}, fsm{fsm_}](
            const outcome::result<sector_storage::PreCommit1Output>
                &maybe_result) {
          if (maybe_result.has_error()) {
            logger->error("Seal pre commit 1 error: {}",
                          maybe_result.error().message());
            OUTCOME_EXCEPT(
                fsm->send(info, SealingEvent::kSectorSealPreCommit1Failed, {}));
            return;
          }
          logger->info("Seal pre commit 1 done: {}", info->sector_number);

          std::shared_ptr<SectorPreCommit1Context> context =
              std::make_shared<SectorPreCommit1Context>();
          context->precommit1_output = maybe_result.value();
          context->ticket = ticket.ticket;
          context->epoch = ticket.epoch;
          OUTCOME_EXCEPT(
              fsm->send(info, SealingEvent::kSectorPreCommit1, context));
        },
        info->sealingPriority());

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handlePreCommit2(
      const std::shared_ptr<SectorInfo> &info) {
    logger_->info("PreCommit 2 sector {}", info->sector_number);
    sealer_->sealPreCommit2(
        minerSector(info->sector_type, info->sector_number),
        info->precommit1_output,
        [logger{logger_}, info, fsm{fsm_}](
            const outcome::result<sector_storage::SectorCids> &maybe_cids) {
          if (maybe_cids.has_error()) {
            logger->error("Seal pre commit 2 error: {}",
                          maybe_cids.error().message());
            OUTCOME_EXCEPT(
                fsm->send(info, SealingEvent::kSectorSealPreCommit2Failed, {}));
            return;
          }
          logger->info("Seal pre commit 2 done: {}", info->sector_number);

          std::shared_ptr<SectorPreCommit2Context> context =
              std::make_shared<SectorPreCommit2Context>();
          context->unsealed = maybe_cids.value().unsealed_cid;
          context->sealed = maybe_cids.value().sealed_cid;

          OUTCOME_EXCEPT(
              fsm->send(info, SealingEvent::kSectorPreCommit2, context));
        },
        info->sealingPriority());

    return outcome::success();
  }

  outcome::result<boost::optional<SealingImpl::PreCommitParams>>
  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  SealingImpl::getPreCommitParams(const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());

    const auto maybe_error = checks::checkPrecommit(
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
    OUTCOME_TRY(policy_expiration, policy_->expiration(info->pieces));
    const auto min_expiration = info->ticket_epoch
                                + kMaxPreCommitRandomnessLookback
                                + seal_duration + kMinSectorExpiration;

    if (policy_expiration < min_expiration) {
      policy_expiration = min_expiration;
    }

    const auto max_expiration = head->epoch() + kPreCommitChallengeDelay
                                + kMaxSectorExpirationExtension;
    if (policy_expiration > max_expiration) {
      policy_expiration = max_expiration;
    }

    SectorPreCommitInfo params;
    params.expiration = policy_expiration;
    params.sector = info->sector_number;
    params.registered_proof = info->sector_type;
    params.sealed_cid = info->comm_r.get();
    params.seal_epoch = info->ticket_epoch;
    params.deal_ids = info->getDealIDs();

    OUTCOME_TRY(collateral,
                api_->StateMinerPreCommitDepositForPower(
                    miner_address_, params, head->key));

    return SealingImpl::PreCommitParams{
        std::move(params), collateral, head->key};
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> SealingImpl::handlePreCommitting(
      const std::shared_ptr<SectorInfo> &info) {
    logger_->info("PreCommitting sector {}", info->sector_number);

    if (config_.batch_pre_commits) {
      FSM_SEND(info, SealingEvent::kSectorBatchSend);
      return outcome::success();
    }

    const auto maybe_precommit_params = getPreCommitParams(info);
    if (maybe_precommit_params.has_error()) {
      logger_->error("PreCommitting sector {} params: {:#}",
                     info->sector_number,
                     maybe_precommit_params.error().message());
      FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
      return outcome::success();
    }
    const auto &precommit_params{maybe_precommit_params.value()};
    if (not precommit_params.has_value()) {
      return outcome::success();
    }

    const auto maybe_params = codec::cbor::encode(precommit_params->info);
    if (maybe_params.has_error()) {
      logger_->error("could not serialize pre-commit sector parameters: {:#}",
                     maybe_params.error().message());
      FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
      return outcome::success();
    }

    const auto maybe_minfo =
        api_->StateMinerInfo(miner_address_, precommit_params->key);
    if (maybe_params.has_error()) {
      logger_->error("API StateMinerInfo error: {:#}",
                     maybe_minfo.error().message());
      FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
      return outcome::success();
    }

    const TokenAmount good_funds =
        precommit_params->deposit + fee_config_->max_precommit_gas_fee;
    const auto maybe_address =
        address_selector_(maybe_minfo.value(), good_funds, api_);
    if (maybe_address.has_error()) {
      logger_->error("no good address to send precommit message from: {:#}",
                     maybe_minfo.error().message());
      FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
      return outcome::success();
    }
    api_->MpoolPushMessage(
        [fsm{fsm_},
         logger{logger_},
         info,
         precommit_params,
         self{shared_from_this()}](
            const outcome::result<api::SignedMessage> &maybe_signed_message) {
          if (maybe_signed_message.has_error()) {
            logger->error("pushing message to mpool: {}",
                          maybe_signed_message.error().message());
            OUTCOME_EXCEPT(
                fsm->send(info, SealingEvent::kSectorChainPreCommitFailed, {}));
            return;
          }
          std::shared_ptr<SectorPreCommittedContext> context =
              std::make_shared<SectorPreCommittedContext>();
          context->precommit_message = maybe_signed_message.value().getCid();
          context->precommit_deposit = precommit_params->deposit;
          context->precommit_info = precommit_params->info;
          OUTCOME_EXCEPT(
              fsm->send(info, SealingEvent::kSectorPreCommitted, context));
        },
        vm::message::UnsignedMessage(miner_address_,
                                     maybe_address.value(),
                                     0,
                                     precommit_params->deposit,
                                     fee_config_->max_precommit_gas_fee,
                                     {},
                                     PreCommitSector::Number,
                                     MethodParams{maybe_params.value()}),
        kPushNoSpec);

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleSubmitPreCommitBatch(
      const std::shared_ptr<SectorInfo> &info) {
    if (not(info->comm_d.has_value() and info->comm_r.has_value())) {
      logger_->error("sector {} had nil commR or commD", info->sector_number);
      FSM_SEND(info, SealingEvent::kSectorSealPreCommit1Failed);
      return outcome::success();
    }

    const auto maybe_precommit_params = getPreCommitParams(info);
    if (maybe_precommit_params.has_error()) {
      logger_->error("PreCommitting sector {} params: {:#}",
                     info->sector_number,
                     maybe_precommit_params.error().message());
      FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
      return outcome::success();
    }
    const auto &precommit_params{maybe_precommit_params.value()};
    if (not precommit_params.has_value()) {
      return outcome::success();
    }

    logger_->info("submitting precommit for sector: {}", info->sector_number);
    const auto maybe_error = precommit_batcher_->addPreCommit(
        *info,
        precommit_params->deposit,
        precommit_params->info,
        [=](const outcome::result<CID> &maybe_cid) -> void {
          if (maybe_cid.has_error()) {
            logger_->error("submitting message to precommit batcher: {}",
                           maybe_cid.error().message());
            OUTCOME_EXCEPT(fsm_->send(
                info, SealingEvent::kSectorChainPreCommitFailed, {}));
            return;
          }
          std::shared_ptr<SectorPreCommittedBatchContext> context =
              std::make_shared<SectorPreCommittedBatchContext>();
          context->precommit_message = maybe_cid.value();
          OUTCOME_EXCEPT(fsm_->send(
              info, SealingEvent::kSectorPreCommittedBatch, context));
        });

    if (maybe_error.has_error()) {
      logger_->error("queuing precommit batch failed: {:#}",
                     maybe_error.error().message());
      FSM_SEND(info, SealingEvent::kSectorChainPreCommitFailed);
    }
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

    api_->StateWaitMsg(
        [info, this](auto &&maybe_lookup) {
          if (maybe_lookup.has_error()) {
            logger_->error("sector precommit failed: {}",
                           maybe_lookup.error().message());
            OUTCOME_EXCEPT(fsm_->send(
                info, SealingEvent::kSectorChainPreCommitFailed, {}));
            return;
          }

          if (maybe_lookup.value().receipt.exit_code != vm::VMExitCode::kOk) {
            logger_->error("sector precommit failed: exit code is {}",
                           maybe_lookup.value().receipt.exit_code);
            OUTCOME_EXCEPT(fsm_->send(
                info, SealingEvent::kSectorChainPreCommitFailed, {}));
            return;
          }

          logger_->info("Seal pre commit done: {}", info->sector_number);
          std::shared_ptr<SectorPreCommitLandedContext> context =
              std::make_shared<SectorPreCommitLandedContext>();
          context->tipset_key = maybe_lookup.value().tipset;

          OUTCOME_EXCEPT(
              fsm_->send(info, SealingEvent::kSectorPreCommitLanded, context));
        },
        info->precommit_message.value(),
        kMessageConfidence,
        api::kLookbackNoLimit,
        true);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleWaitSeed(
      const std::shared_ptr<SectorInfo> &info) {
    logger_->info("Wait Seed sector {}", info->sector_number);
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
        [=](const TipsetCPtr &,
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
        [=](const TipsetCPtr &) -> outcome::result<void> {
          logger_->warn("revert in interactive commit sector step");
          // TODO(ortyomka): cancel running and restart
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

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> SealingImpl::handleComputeProof(
      const std::shared_ptr<SectorInfo> &info) {
    if (info->message.has_value()) {
      logger_->warn(
          "sector {} entered committing state with a commit message cid",
          info->sector_number);

      // TODO(ortyomka): maybe async call, it's long
      OUTCOME_TRY(message,
                  api_->StateSearchMsg(
                      {}, *(info->message), api::kLookbackNoLimit, true));

      if (message.has_value()) {
        FSM_SEND(info, SealingEvent::kSectorRetryCommitWait);
        return outcome::success();
      }
    }

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

    const auto sector_ref = minerSector(info->sector_type, info->sector_number);
    const auto next_cb =
        [info,
         fsm{fsm_},
         logger{logger_},
         sealer{sealer_},
         api{api_},
         miner_address{miner_address_},
         sector_ref](const outcome::result<sector_storage::Commit1Output>
                         &maybe_commit_1_output) mutable {
          if (maybe_commit_1_output.has_error()) {
            logger->error("computing seal proof failed(1): {}",
                          maybe_commit_1_output.error().message());
            OUTCOME_EXCEPT(
                fsm->send(info, SealingEvent::kSectorComputeProofFailed, {}));
            return;
          }
          logger->info("Commit 2 sector {}", info->sector_number);

          sealer->sealCommit2(
              sector_ref,
              maybe_commit_1_output.value(),
              [logger,
               info,
               api,
               fsm,
               miner_address,
               proof_engine{sealer->getProofEngine()}](
                  const outcome::result<sector_storage::Proof> &maybe_proof) {
                if (maybe_proof.has_error()) {
                  logger->error("computing seal proof failed(2): {}",
                                maybe_proof.error().message());
                  OUTCOME_EXCEPT(fsm->send(
                      info, SealingEvent::kSectorComputeProofFailed, {}));
                  return;
                }
                logger->info("Seal commit done: {}", info->sector_number);

                const auto maybe_head = api->ChainHead();
                if (maybe_head.has_error()) {
                  logger->error("computing seal proof api error: {}",
                                maybe_head.error().message());
                  OUTCOME_EXCEPT(fsm->send(
                      info, SealingEvent::kSectorComputeProofFailed, {}));
                  return;
                }

                const auto maybe_error =
                    checks::checkCommit(miner_address,
                                        info,
                                        maybe_proof.value(),
                                        maybe_head.value()->key,
                                        api,
                                        proof_engine);
                if (maybe_error.has_error()) {
                  logger->error("commit check error: {}",
                                maybe_error.error().message());
                  OUTCOME_EXCEPT(fsm->send(
                      info, SealingEvent::kSectorComputeProofFailed, {}));
                  return;
                }

                std::shared_ptr<SectorComputeProofContext> context =
                    std::make_shared<SectorComputeProofContext>();
                context->proof = maybe_proof.value();
                OUTCOME_EXCEPT(fsm->send(
                    info, SealingEvent::kSectorComputeProof, context));
              },
              info->sealingPriority());
        };

    sealer_->sealCommit1(sector_ref,
                         info->ticket,
                         info->seed,
                         info->getPieceInfos(),
                         cids,
                         next_cb,
                         info->sealingPriority());

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleCommitting(
      const std::shared_ptr<SectorInfo> &info) {
    logger_->info("Commiting sector {}", info->sector_number);
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

    // TODO(ortyomka): check seed / ticket are up to date
    api_->MpoolPushMessage(
        [fsm{fsm_}, logger{logger_}, info](
            const outcome::result<api::SignedMessage> &maybe_signed_msg) {
          if (maybe_signed_msg.has_error()) {
            logger->error("pushing message to mpool: {}",
                          maybe_signed_msg.error().message());
            OUTCOME_EXCEPT(
                fsm->send(info, SealingEvent::kSectorCommitFailed, {}));
            return;
          }

          std::shared_ptr<SectorCommittedContext> context =
              std::make_shared<SectorCommittedContext>();
          context->message = maybe_signed_msg.value().getCid();
          OUTCOME_EXCEPT(
              fsm->send(info, SealingEvent::kSectorCommitted, context));
        },
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
    logger_->info("Committed sector {}", info->sector_number);

    api_->StateWaitMsg(
        [=](auto &&maybe_message_lookup) {
          if (maybe_message_lookup.has_error()) {
            logger_->error("failed to wait for porep inclusion: {}",
                           maybe_message_lookup.error().message());
            OUTCOME_EXCEPT(
                fsm_->send(info, SealingEvent::kSectorCommitFailed, {}));
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

          api_->StateSectorGetInfo(
              [=](auto &&maybe_error) {
                if (maybe_error.has_error()) {
                  logger_->error(
                      "proof validation failed, sector not found in sector set "
                      "after "
                      "cron: "
                      "{}",
                      maybe_error.error().message());
                  OUTCOME_EXCEPT(
                      fsm_->send(info, SealingEvent::kSectorCommitFailed, {}));
                  return;
                }

                if (!maybe_error.value()) {
                  logger_->error(
                      "proof validation failed, sector not found in sector set "
                      "after "
                      "cron");
                  OUTCOME_EXCEPT(
                      fsm_->send(info, SealingEvent::kSectorCommitFailed, {}));
                  return;
                }

                OUTCOME_EXCEPT(
                    fsm_->send(info, SealingEvent::kSectorProving, {}));
              },
              miner_address_,
              info->sector_number,
              maybe_message_lookup.value().tipset);
        },
        info->message.get(),
        kMessageConfidence,
        api::kLookbackNoLimit,
        true);

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleFinalizeSector(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO(ortyomka): Maybe wait for some finality
    logger_->info("Finalize sector {}", info->sector_number);

    sealer_->finalizeSector(
        minerSector(info->sector_type, info->sector_number),
        info->keepUnsealedRanges(),
        [fsm{fsm_}, info, logger{logger_}](
            const outcome::result<void> &maybe_error) {
          if (maybe_error.has_error()) {
            logger->error("finalize sector: {}", maybe_error.error().message());
            OUTCOME_EXCEPT(
                fsm->send(info, SealingEvent::kSectorFinalizeFailed, {}));
            return;
          }
          logger->info("Finalize done: {}", info->sector_number);

          OUTCOME_EXCEPT(fsm->send(info, SealingEvent::kSectorFinalized, {}));
        },
        info->sealingPriority());

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleProvingSector(
      const std::shared_ptr<SectorInfo> &info) {
    // TODO(ortyomka): track sector health / expiration

    logger_->info("Proving sector {}", info->sector_number);

    // TODO(ortyomka): release unsealed

    // TODO(ortyomka): Watch termination
    // TODO(ortyomka): Auto-extend if set

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleReplicaUpdate(
      const std::shared_ptr<SectorInfo> &info) {
    logger_->info("Performing replica update for the sector {}",
                  info->sector_number);

    const auto maybe_error = checks::checkPieces(miner_address_, info, api_);
    if (maybe_error.has_error()) {
      if (maybe_error == outcome::failure(ChecksError::kInvalidDeal)) {
        logger_->error("invalid dealIDs in sector {}", info->sector_number);
        std::shared_ptr<SectorInvalidDealIDContext> context =
            std::make_shared<SectorInvalidDealIDContext>();
        context->return_state = SealingState::kUpdateReplica;
        FSM_SEND_CONTEXT(info, SealingEvent::kSectorInvalidDealIDs, context);
        return outcome::success();
      }
      if (maybe_error == outcome::failure(ChecksError::kExpiredDeal)) {
        logger_->error("expired dealIDs in sector {}", info->sector_number);
        FSM_SEND(info, SealingEvent::kSectorDealsExpired);
        return outcome::success();
      }
    }

    sealer_->replicaUpdate(
        minerSector(info->sector_type, info->sector_number),
        info->getPieceInfos(),
        [info{info}, fsm{fsm_}, logger{logger_}](
            const outcome::result<ReplicaUpdateOut> &replica_update_out) {
          if (replica_update_out.has_error()) {
            logger->error("ReplicaUpdate error: {}",
                          replica_update_out.error().message());
            OUTCOME_EXCEPT(
                fsm->send(info, SealingEvent::kSectorUpdateReplicaFailed, {}));
            return;
          }

          auto context = std::make_shared<SectorReplicaUpdateContext>();
          context->update_sealed = replica_update_out.value().sealed_cid;
          context->update_unsealed = replica_update_out.value().unsealed_cid;
          OUTCOME_EXCEPT(
              fsm->send(info, SealingEvent::kSectorReplicaUpdate, context));
        },
        info->sealingPriority());
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleProveReplicaUpdate(
      const std::shared_ptr<SectorInfo> &info) {
    if (not info->update_sealed.has_value()
        || not info->update_sealed.has_value()) {
      logger_->error(
          "invalid sector {} without Update sealed or Update unsealed",
          info->sector_number);
      return outcome::success();
    }

    if (not info->comm_r.has_value()) {
      logger_->error("invalid sector {} without CommR", info->sector_number);
      return outcome::success();
    }

    OUTCOME_TRY(head, api_->ChainHead());
    OUTCOME_TRY(
        active,
        sectorActive(api_, miner_address_, head->key, info->sector_number));

    if (not active) {
      logger_->error(
          "sector marked for upgrade {} no longer active, aborting upgrade",
          info->sector_number);
      FSM_SEND(info, SealingEvent::kSectorAbortUpgrade);
      return outcome::success();
    }

    const auto sector{minerSector(info->sector_type, info->sector_number)};
    sealer_->proveReplicaUpdate1(
        sector,
        info->comm_r.get(),
        info->update_sealed.get(),
        info->update_unsealed.get(),
        [sector, info, self{shared_from_this()}](const auto &maybe_res) {
          if (maybe_res.has_error()) {
            self->logger_->error(
                "prove replica update (1) for sector {} failed: {}",
                info->sector_number,
                maybe_res.error().message());
            OUTCOME_EXCEPT(self->fsm_->send(
                info, SealingEvent::kSectorProveReplicaUpdateFailed, {}));
            return;
          }

          const auto maybe_error =
              checks::checkPieces(self->miner_address_, info, self->api_);
          if (maybe_error.has_error()) {
            if (maybe_error == outcome::failure(ChecksError::kInvalidDeal)) {
              self->logger_->error("invalid dealIDs in sector {}",
                                   info->sector_number);
              std::shared_ptr<SectorInvalidDealIDContext> context =
                  std::make_shared<SectorInvalidDealIDContext>();
              context->return_state = SealingState::kProveReplicaUpdate;
              OUTCOME_EXCEPT(self->fsm_->send(
                  info, SealingEvent::kSectorInvalidDealIDs, context));
              return;
            }
            if (maybe_error == outcome::failure(ChecksError::kExpiredDeal)) {
              self->logger_->error("expired dealIDs in sector {}",
                                   info->sector_number);
              OUTCOME_EXCEPT(self->fsm_->send(
                  info, SealingEvent::kSectorDealsExpired, {}));
              return;
            }
            self->logger_->error("check pieces in sector {} error:",
                                 info->sector_number,
                                 maybe_error.error().message());
            return;
          }

          self->sealer_->proveReplicaUpdate2(
              sector,
              info->comm_r.get(),
              info->update_sealed.get(),
              info->update_unsealed.get(),
              maybe_res.value(),
              [self, info](const auto &maybe_res) {
                if (maybe_res.has_error()) {
                  self->logger_->error(
                      "prove replica update (2) for sector {} failed: {}",
                      info->sector_number,
                      maybe_res.error().message());
                  OUTCOME_EXCEPT(self->fsm_->send(
                      info, SealingEvent::kSectorProveReplicaUpdateFailed, {}));
                  return;
                }

                auto context =
                    std::make_shared<SectorProveReplicaUpdateContext>();
                context->proof = maybe_res.value();
                OUTCOME_EXCEPT(self->fsm_->send(
                    info, SealingEvent::kSectorProveReplicaUpdate, context));
              },
              info->sealingPriority());
        },
        info->sealingPriority());

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleSubmitReplicaUpdate(
      const std::shared_ptr<SectorInfo> &info) {
    OUTCOME_TRY(head, api_->ChainHead());

    const auto maybe_error = checks::checkUpdate(
        miner_address_, info, head->key, api_, sealer_->getProofEngine());
    if (maybe_error.has_error()) {
      logger_->error("SubmitReplicaUpdate check update error: {}",
                     maybe_error.error().message());
      FSM_SEND(info, SealingEvent::kSectorSubmitReplicaUpdateFailed);
      return outcome::success();
    }

    OUTCOME_TRY(state_partition,
                api_->StateSectorPartition(
                    miner_address_, info->sector_number, head->key));

    const auto maybe_proof = getRegisteredUpdateProof(info->sector_type);
    if (maybe_proof.has_error()) {
      FSM_SEND(info, SealingEvent::kSectorSubmitReplicaUpdateFailed);
      return outcome::success();
    }
    const auto &proof{maybe_proof.value()};

    ProveReplicaUpdates::Params params{
        {ReplicaUpdate{.sector = info->sector_number,
                       .deadline = state_partition.deadline,
                       .partition = state_partition.partition,
                       .comm_r = info->update_sealed.get(),
                       .deals = info->getDealIDs(),
                       .update_type = proof,
                       .proof = info->update_proof.get()}}};

    const auto maybe_encoded = codec::cbor::encode(params);
    if (maybe_encoded.has_error()) {
      logger_->error("could not serialize update sector parameters: {:#}",
                     maybe_encoded.error().message());
      FSM_SEND(info, SealingEvent::kSectorSubmitReplicaUpdateFailed);
      return outcome::success();
    }

    OUTCOME_TRY(chain_info,
                api_->StateSectorGetInfo(
                    miner_address_, info->sector_number, head->key));
    OUTCOME_TRY(seal_proof, getCurrentSealProof());

    SectorPreCommitInfo virtual_pci{
        .registered_proof = seal_proof,
        .sector = info->sector_number,
        .sealed_cid = info->update_sealed.get(),
        .seal_epoch = 0,
        .deal_ids = info->getDealIDs(),
        .expiration = chain_info->expiration,
    };

    OUTCOME_TRY(collateral,
                api_->StateMinerInitialPledgeCollateral(
                    miner_address_, virtual_pci, head->key));

    collateral -= chain_info->init_pledge;
    if (collateral < 0) {
      collateral = 0;
    }

    const auto good_funds = collateral + fee_config_->max_commit_gas_fee;

    OUTCOME_TRY(miner_info, api_->StateMinerInfo(miner_address_, head->key));

    const auto maybe_address = address_selector_(miner_info, good_funds, api_);
    if (maybe_address.has_error()) {
      FSM_SEND(info, SealingEvent::kSectorCommitFailed);
      return outcome::success();
    }

    api_->MpoolPushMessage(
        [fsm{fsm_}, logger{logger_}, info, self{shared_from_this()}](
            const outcome::result<api::SignedMessage> &maybe_signed_message) {
          if (maybe_signed_message.has_error()) {
            logger->error(
                "handleSubmitReplicaUpdate: error sending message: {}",
                maybe_signed_message.error().message());
            OUTCOME_EXCEPT(fsm->send(
                info, SealingEvent::kSectorSubmitReplicaUpdateFailed, {}));
            return;
          }
          std::shared_ptr<SectorReplicaUpdateSubmittedContext> context =
              std::make_shared<SectorReplicaUpdateSubmittedContext>();
          context->message = maybe_signed_message.value().getCid();
          OUTCOME_EXCEPT(fsm->send(
              info, SealingEvent::kSectorReplicaUpdateSubmitted, context));
        },
        vm::message::UnsignedMessage(miner_address_,
                                     maybe_address.value(),
                                     0,
                                     collateral,
                                     fee_config_->max_commit_gas_fee,
                                     {},
                                     ProveReplicaUpdates::Number,
                                     MethodParams{maybe_encoded.value()}),
        kPushNoSpec);

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleReplicaUpdateWait(
      const std::shared_ptr<SectorInfo> &info) {
    if (not info->update_message.has_value()) {
      logger_->error(
          "handleReplicaUpdateWait: no replica update message cid recorded");
      FSM_SEND(info, SealingEvent::kSectorSubmitReplicaUpdateFailed);
      return outcome::success();
    }

    api_->StateWaitMsg(
        [self{shared_from_this()}, info](const auto &maybe_result) {
          if (maybe_result.has_error()) {
            self->logger_->error(
                "handleReplicaUpdateWait: failed to wait for message: {}",
                maybe_result.error().message());
            OUTCOME_EXCEPT(self->fsm_->send(
                info, SealingEvent::kSectorSubmitReplicaUpdateFailed, {}));
            return;
          }
          const auto &msg{maybe_result.value()};

          switch (msg.receipt.exit_code) {
            case vm::VMExitCode::kOk:
              break;
            case vm::VMExitCode::kSysErrInsufficientFunds:
            case vm::VMExitCode::kSysErrOutOfGas:
              self->logger_->error("gas estimator was wrong or out of funds");
            default:
              OUTCOME_EXCEPT(self->fsm_->send(
                  info, SealingEvent::kSectorSubmitReplicaUpdateFailed, {}));
              return;
          }

          const auto maybe_sector_info = self->api_->StateSectorGetInfo(
              self->miner_address_, info->sector_number, msg.tipset);
          if (maybe_sector_info.has_error()) {
            self->logger_->error(
                "error calling StateSectorGetInfo for replaced sector: {}",
                maybe_sector_info.error().message());
            OUTCOME_EXCEPT(self->fsm_->send(
                info, SealingEvent::kSectorSubmitReplicaUpdateFailed, {}));
            return;
          }
          const auto &sector_info{maybe_sector_info.value()};
          if (not sector_info.has_value()) {
            self->logger_->error("api err sector {} not found: {}",
                                 info->sector_number,
                                 maybe_sector_info.error().message());
            OUTCOME_EXCEPT(self->fsm_->send(
                info, SealingEvent::kSectorSubmitReplicaUpdateFailed, {}));
            return;
          }

          if (sector_info.get().sealed_cid == info->update_sealed.get()) {
            self->logger_->error(
                "mismatch of expected onchain sealed cid after replica update, "
                "expected {} got {}",
                info->update_sealed.get().toString().value(),
                sector_info.get().sealed_cid.toString().value());
            OUTCOME_EXCEPT(
                self->fsm_->send(info, SealingEvent::kSectorAbortUpgrade, {}));
            return;
          }

          OUTCOME_EXCEPT(self->fsm_->send(
              info, SealingEvent::kSectorReplicaUpdateLanded, {}));
          return;
        },
        info->update_message.get(),
        kMessageConfidence,
        api::kLookbackNoLimit,
        true);

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleFinalizeReplicaUpdate(
      const std::shared_ptr<SectorInfo> &info) {
    sealer_->finalizeReplicaUpdate(
        minerSector(info->sector_type, info->sector_number),
        info->keepUnsealedRanges(),
        [fsm{fsm_}, info, logger{logger_}](
            const outcome::result<void> &maybe_error) {
          if (maybe_error.has_error()) {
            logger->error("finalize replica update: {}",
                          maybe_error.error().message());
            OUTCOME_EXCEPT(
                fsm->send(info, SealingEvent::kSectorFinalizeFailed, {}));
            return;
          }

          OUTCOME_EXCEPT(fsm->send(info, SealingEvent::kSectorFinalized, {}));
        },
        info->sealingPriority());

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleUpdateActivating(
      const std::shared_ptr<SectorInfo> &info) {
    api_->StateWaitMsg(
        [self{shared_from_this()}, info](const auto &maybe_result) {
          const auto cb = [=](const outcome::result<void> &error) {
            self->logger_->error("handleUpdateActivating error: {}",
                                 error.error().message());
            self->scheduler_->schedule(
                [self, info]() {
                  self->context_->post(
                      [=]() { self->handleUpdateActivating(info).value(); });
                },
                std::chrono::minutes(1));
          };

          OUTCOME_CB(auto msg, maybe_result);

          OUTCOME_CB(auto head, self->api_->ChainHead());

          ChainEpoch targetHeight =
              msg.height + kChainFinality + kInteractivePoRepConfidence;

          OUTCOME_CB1(self->events_->chainAt(
              [self, info](const TipsetCPtr &,
                           ChainEpoch current_height) -> outcome::result<void> {
                OUTCOME_TRY(self->fsm_->send(
                    info, SealingEvent::kSectorUpdateActive, {}));
                return outcome::success();
              },
              [self](const TipsetCPtr &) -> outcome::result<void> {
                self->logger_->warn("revert in handleUpdateActivating");
                return outcome::success();
              },
              kInteractivePoRepConfidence,
              targetHeight));
        },
        info->update_message.get(),
        kMessageConfidence,
        api::kLookbackNoLimit,
        true);

    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleReleaseSectorKey(
      const std::shared_ptr<SectorInfo> &info) {
    const auto maybe_error = sealer_->releaseSectorKey(
        minerSector(info->sector_type, info->sector_number));
    if (maybe_error.has_error()) {
      logger_->error("handleReleaseSectorKey error: {}",
                     maybe_error.error().message());
      FSM_SEND(info, SealingEvent::kSectorReleaseKeyFailed);
      return outcome::success();
    }

    FSM_SEND(info, SealingEvent::kSectorKeyReleased);
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

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
        return outcome::success();  // TODO(ortyomka): remove when the actor
                                    // allows re-precommit
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
    // TODO(ortyomka): Check sector files

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

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
      // TODO(ortyomka): maybe async call, it's long
      auto maybe_message_wait = api_->StateSearchMsg(
          {}, info->message.value(), api::kLookbackNoLimit, true);

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

    // TODO(ortyomka): Check sector files

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
    // TODO(ortyomka): Check sector files

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
    return handleRecoverDealWithFail(info, SealingEvent::kSectorRemove);
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> SealingImpl::handleRecoverDealWithFail(
      const std::shared_ptr<SectorInfo> &info, SealingEvent fail_event) {
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

      if (proposal->provider != miner_address_) {
        logger_->warn(
            "piece {} (of {}) of sector {} refers deal {} with wrong provider: "
            "{} != {}",
            i,
            info->pieces.size(),
            info->sector_number,
            piece.deal_info->deal_id,
            encodeToString(miner_address_),
            encodeToString(proposal->provider));
        to_fix.push_back(i);
        continue;
      }

      if (proposal->piece_cid != piece.piece.cid) {
        OUTCOME_TRY(expected_cid, proposal->piece_cid.toString());
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
      if (proposal->piece_size != piece.piece.size) {
        logger_->warn(
            "piece {} (of {}) of sector {} refers deal {} with different size: "
            "{} != {}",
            i,
            info->pieces.size(),
            info->sector_number,
            piece.deal_info->deal_id,

            piece.piece.size,

            proposal->piece_size);
        to_fix.push_back(i);
        continue;
      }

      if (head->height() >= proposal->start_epoch) {
        // TODO(ortyomka): [FIL-382] try to remove the offending pieces
        logger_->error(
            "can't fix sector deals: piece {} (of {}) of sector {} refers "
            "expired deal {} - should start at {}, head {}",
            i,
            info->pieces.size(),
            info->sector_number,
            piece.deal_info->deal_id,
            proposal->start_epoch,
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
      // deal proposal must present if deal is on chain
      assert(piece.deal_info->deal_proposal.has_value());
      const auto maybe_result =
          deal_info->getCurrentDealInfo(piece.deal_info->deal_proposal.value(),
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
        FSM_SEND(info, fail_event);
        return outcome::success();
      }

      // TODO(ortyomka): [FIL-382] try to recover

      logger_->error("sector {}: deals expired or unrecoverable",
                     info->sector_number);
      FSM_SEND(info, fail_event);
      return outcome::success();
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

    // TODO(ortyomka): maybe async call, it's long
    OUTCOME_TRY(message,
                api_->StateWaitMsg(info->fault_report_message.get(),
                                   kMessageConfidence,
                                   api::kLookbackNoLimit,
                                   true));

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

  outcome::result<void> SealingImpl::handleSnapDealsAddPieceFailed(
      const std::shared_ptr<SectorInfo> &info) {
    FSM_SEND(info, SealingEvent::kSectorRetryWaitDeals);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleReplicaUpdateFailed(
      const std::shared_ptr<SectorInfo> &info) {
    auto cb = [self{shared_from_this()}, info]() -> outcome::result<void> {
      const auto maybe_head = self->api_->ChainHead();
      if (maybe_head.has_error()) {
        self->logger_->error(
            "handleReplicaUpdateFailed: api error, not proceeding: {}",
            maybe_head.error().message());
        return outcome::success();
      }
      const auto &head{maybe_head.value()};

      const auto maybe_error =
          checks::checkUpdate(self->miner_address_,
                              info,
                              head->key,
                              self->api_,
                              self->sealer_->getProofEngine());
      if (maybe_error.has_error()) {
        if (maybe_error
            == outcome::failure(checks::ChecksError::kBadUpdateReplica)) {
          return self->fsm_->send(
              info, SealingEvent::kSectorRetryReplicaUpdate, {});
        }
        if (maybe_error
            == outcome::failure(checks::ChecksError::kBadUpdateProof)) {
          return self->fsm_->send(
              info, SealingEvent::kSectorRetryProveReplicaUpdate, {});
        }
        if (maybe_error
            == outcome::failure(checks::ChecksError::kInvalidDeal)) {
          return self->fsm_->send(
              info, SealingEvent::kSectorInvalidDealIDs, {});
        }
        if (maybe_error
            == outcome::failure(checks::ChecksError::kExpiredDeal)) {
          return self->fsm_->send(info, SealingEvent::kSectorDealsExpired, {});
        }
        self->logger_->error("sanity check error, not proceeding: {}",
                             maybe_error.error().message());
        return outcome::success();
      }
      const auto maybe_active = sectorActive(
          self->api_, self->miner_address_, head->key, info->sector_number);
      if (maybe_active.has_error()) {
        self->logger_->error(
            "sector active check: api error, not proceeding: {}",
            maybe_active.error().message());
        return outcome::success();
      }
      if (not maybe_active.value()) {
        self->logger_->error(
            "sector marked for upgrade {} no longer active, aborting upgrade",
            info->sector_number);
        return self->fsm_->send(info, SealingEvent::kSectorAbortUpgrade, {});
      }

      self->scheduler_->schedule(
          [self, info] {
            OUTCOME_EXCEPT(self->fsm_->send(
                info, SealingEvent::kSectorRetrySubmitReplicaUpdate, {}));
          },
          getWaitingTime());
      return outcome::success();
    };

    if (info->update_message.has_value()) {
      api_->StateSearchMsg(
          [self{shared_from_this()}, cb, info](const auto &value) {
            if (value.has_error()) {
              self->scheduler_->schedule(
                  [self, info] {
                    OUTCOME_EXCEPT(self->fsm_->send(
                        info,
                        SealingEvent::kSectorRetrySubmitReplicaUpdateWait,
                        {}));
                  },
                  getWaitingTime());
              return;
            }

            if (not value.value().has_value()) {
              OUTCOME_EXCEPT(self->fsm_->send(
                  info, SealingEvent::kSectorRetrySubmitReplicaUpdateWait, {}));
              return;
            }
            const auto &msg{value.value().value()};
            switch (msg.receipt.exit_code) {
              case vm::VMExitCode::kOk:
                OUTCOME_EXCEPT(self->fsm_->send(
                    info,
                    SealingEvent::kSectorRetrySubmitReplicaUpdateWait,
                    {}));
                return;
              case vm::VMExitCode::kSysErrOutOfGas:
                OUTCOME_EXCEPT(self->fsm_->send(
                    info, SealingEvent::kSectorRetrySubmitReplicaUpdate, {}));
                return;
              default:
                break;  // something else goes wrong
            }

            self->context_->post([self, cb]() {
              auto maybe_error = cb();
              if (maybe_error.has_error()) {
                self->logger_->error("fsm error: {}",
                                     maybe_error.error().message());
              }
            });
          },
          TipsetKey{},
          info->update_message.get(),
          api::kLookbackNoLimit,
          true);
      return outcome::success();
    }

    return cb();
  }

  outcome::result<void> SealingImpl::handleSnapDealsDealsExpired(
      const std::shared_ptr<SectorInfo> &info) {
    if (not info->update) {
      return ERROR_TEXT(
          "should never reach AbortUpgrade as a non-update sector");
    }

    FSM_SEND(info, SealingEvent::kSectorAbortUpgrade);
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleSnapDealsRecoverDealIDs(
      const std::shared_ptr<SectorInfo> &info) {
    return handleRecoverDealWithFail(info, SealingEvent::kSectorAbortUpgrade);
  }

  outcome::result<void> SealingImpl::handleAbortUpgrade(
      const std::shared_ptr<SectorInfo> &info) {
    if (not info->update) {
      return ERROR_TEXT(
          "should never reach AbortUpgrade as a non-update sector");
    }

    OUTCOME_TRY(sealer_->releaseReplicaUpgrade(
        minerSector(info->sector_type, info->sector_number)));

    FSM_SEND_CONTEXT(info,
                     SealingEvent::kSectorRevertUpgradeToProving,
                     std::make_shared<SectorRevertUpgradeToProvingContext>());
    return outcome::success();
  }

  outcome::result<void> SealingImpl::handleReleaseSectorKeyFailed(
      const std::shared_ptr<SectorInfo> &info) {
    scheduler_->schedule(
        [fsm{fsm_}, info] {
          OUTCOME_EXCEPT(
              fsm->send(info, SealingEvent::kSectorUpdateActive, {}));
        },
        getWaitingTime());
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

  outcome::result<RegisteredSealProof> SealingImpl::getCurrentSealProof()
      const {
    OUTCOME_TRY(miner_info, api_->StateMinerInfo(miner_address_, {}));
    OUTCOME_TRY(version, api_->StateNetworkVersion({}));
    return getPreferredSealProofTypeFromWindowPoStType(
        version, miner_info.window_post_proof_type);
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
    case E::kCannotMarkInactiveSector:
      return "SealingError: cannot mark inactive sector for upgrade";
    case E::kSectorExpirationError:
      return "SealingError: Pointless to upgrade sector. Sector Info "
             "expiration is less than a min deal duration away from current "
             "state";
    default:
      return "SealingError: unknown error";
  }
}
