/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/chain_events/impl/chain_events_impl.hpp"
#include "common/outcome_fmt.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "vm/actor/builtin/methods/miner.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/types/miner/replica_update.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"

namespace fc::markets::storage::chain_events {
  using primitives::RleBitset;
  using primitives::tipset::HeadChangeType;
  using vm::VMExitCode;
  using vm::actor::builtin::states::MinerActorStatePtr;
  using vm::actor::builtin::types::miner::ReplicaUpdate;
  using vm::actor::builtin::types::miner::SectorPreCommitInfo;
  using vm::message::SignedMessage;
  namespace miner = vm::actor::builtin::miner;

  ChainEventsImpl::ChainEventsImpl(std::shared_ptr<FullNodeApi> api,
                                   IsDealPrecommited is_deal_precommited)
      : api_{std::move(api)},
        is_deal_precommited_{std::move(is_deal_precommited)} {
    if (!is_deal_precommited_) {
      is_deal_precommited_ = [api{api_}](const TipsetKey &tsk,
                                         const Address &miner,
                                         DealId deal_id)
          -> outcome::result<boost::optional<SectorNumber>> {
        OUTCOME_TRY(actor, api->StateGetActor(miner, tsk));
        const auto ipld{
            std::make_shared<fc::storage::ipfs::ApiIpfsDatastore>(api)};
        OUTCOME_TRY(network, api->StateNetworkVersion(tsk));
        ipld->actor_version = actorVersion(network);
        OUTCOME_TRY(state, getCbor<MinerActorStatePtr>(ipld, actor.head));
        boost::optional<SectorNumber> result;
        constexpr auto kStop{std::errc::interrupted};
        const auto visit{state->precommitted_sectors.visit(
            [&](auto sector, auto &precommit) -> outcome::result<void> {
              const auto &ids{precommit.info.deal_ids};
              if (std::find(ids.begin(), ids.end(), deal_id) != ids.end()) {
                result = sector;
                return outcome::failure(kStop);
              }
              return outcome::success();
            })};
        if (!visit && visit.error() != kStop) {
          return visit.error();
        }
        return result;
      };
    }
  }

  outcome::result<void> ChainEventsImpl::init() {
    OUTCOME_TRY(chan, api_->ChainNotify());
    channel_ = chan.channel;
    channel_->read(
        [self_weak{weak_from_this()}](
            const boost::optional<std::vector<HeadChange>> &update) -> bool {
          if (auto self = self_weak.lock()) return self->onRead(update);
          return false;
        });
    return outcome::success();
  }

  void ChainEventsImpl::onDealSectorCommitted(const Address &provider,
                                              const DealId &deal_id,
                                              CommitCb cb) {
    constexpr auto kStop{std::errc::interrupted};
    const auto _r{[&]() -> outcome::result<void> {
      OUTCOME_TRY(deal, api_->StateMarketStorageDeal(deal_id, head_->key));
      if (deal.state.sector_start_epoch > 0) {
        cb(outcome::success());
        return outcome::success();
      }
      OUTCOME_TRY(sector, is_deal_precommited_(head_->key, provider, deal_id));
      std::unique_lock lock{watched_events_mutex_};
      if (sector) {
        watched_events_[provider].commits.emplace(*sector, std::move(cb));
      } else {
        watched_events_[provider].precommits.emplace(deal_id, std::move(cb));
      }
      return outcome::success();
    }()};
    if (!_r && _r.error() != kStop) {
      logger_->warn("ChainEventsImpl::onDealSectorCommitted {:#}", _r.error());
    }
  }

  /**
   * Actually sector commit consists of 2 method calls:
   *  1) PreCommitSector with desired provider address and deal id. Parameters
   * contain sector number used in the next call
   *  2) ProveCommitSector with desired provider address and sector number
   */
  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  bool ChainEventsImpl::onRead(
      const boost::optional<std::vector<HeadChange>> &changes) {
    std::unique_lock lock{watched_events_mutex_};
    if (changes) {
      for (const auto &change : changes.get()) {
        if (change.type != HeadChangeType::REVERT) {
          head_ = change.value;
        }
        if (change.type == HeadChangeType::APPLY) {
          for (const auto &block_cid : change.value->key.cids()) {
            auto block_messages = api_->ChainGetBlockMessages(CID{block_cid});
            if (block_messages.has_error()) {
              logger_->error("ChainGetBlockMessages error: "
                             + block_messages.error().message());
              continue;
            }
            for (const auto &message : block_messages.value().bls) {
              auto message_processed = onMessage(message, message.getCid());
              if (message_processed.has_error()) {
                logger_->error("Message process error: "
                               + message_processed.error().message());
              }
            }
            for (const auto &message : block_messages.value().secp) {
              auto message_processed =
                  onMessage(message.message, message.getCid());
              if (message_processed.has_error()) {
                logger_->error("Message process error: "
                               + message_processed.error().message());
              }
            }
          }
        }
      }
    }
    lock.unlock();
    return true;
  };

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> ChainEventsImpl::onMessage(
      const UnsignedMessage &message, const CID &cid) {
    const auto _watch{watched_events_.find(message.to)};
    if (_watch == watched_events_.end()) {
      return outcome::success();
    }
    auto &watch{_watch->second};
    const auto on_deals{[&](const std::vector<DealId> &deals,
                            SectorNumber sector,
                            bool update) {
      for (const auto &deal_id : deals) {
        const auto [begin, end]{watch.precommits.equal_range(deal_id)};
        auto it{begin};
        while (it != end) {
          api_->StateWaitMsg(
              [=, cb{std::move(it->second)}, provider{_watch->first}](auto _r) {
                OUTCOME_CB(auto r, _r);
                if (r.receipt.exit_code != VMExitCode::kOk) {
                  return cb(r.receipt.exit_code);
                }
                if (update) {
                  OUTCOME_CB(
                      auto result,
                      codec::cbor::decode<miner::ProveReplicaUpdates::Result>(
                          r.receipt.return_value));
                  if (!result.has(sector)) {
                    return cb(ERROR_TEXT("ProveReplicaUpdates failed"));
                  }
                  return cb(outcome::success());
                }
                std::unique_lock lock{watched_events_mutex_};
                watched_events_[provider].commits.emplace(sector,
                                                          std::move(cb));
              },
              cid,
              kMessageConfidence,
              api::kLookbackNoLimit,
              true);
          it = watch.precommits.erase(it);
        }
      }
    }};
    const auto on_precommit{[&](const SectorPreCommitInfo &precommit) {
      on_deals(precommit.deal_ids, precommit.sector, false);
    }};
    const auto on_commit{[&](SectorNumber sector) {
      const auto [begin, end]{watch.commits.equal_range(sector)};
      auto it{begin};
      while (it != end) {
        api_->StateWaitMsg(
            [cb{std::move(it->second)}](auto _r) {
              OUTCOME_CB(auto r, _r);
              if (r.receipt.exit_code != vm::VMExitCode::kOk) {
                return cb(r.receipt.exit_code);
              }
              return cb(outcome::success());
            },
            cid,
            kMessageConfidence,
            api::kLookbackNoLimit,
            true);
        it = watch.commits.erase(it);
      }
    }};
    const auto on_update{[&](const ReplicaUpdate &update) {
      on_deals(update.deals, update.sector, true);
    }};
    if (message.method == miner::PreCommitSector::Number) {
      OUTCOME_TRY(
          param,
          codec::cbor::decode<miner::PreCommitSector::Params>(message.params));
      on_precommit(param);
    } else if (message.method == miner::PreCommitSectorBatch::Number) {
      OUTCOME_TRY(param,
                  codec::cbor::decode<miner::PreCommitSectorBatch::Params>(
                      message.params));
      for (const auto &precommit : param.sectors) {
        on_precommit(precommit);
      }
    } else if (message.method == miner::ProveCommitSector::Number) {
      OUTCOME_TRY(param,
                  codec::cbor::decode<miner::ProveCommitSector::Params>(
                      message.params));
      on_commit(param.sector);
    } else if (message.method == miner::ProveCommitAggregate::Number) {
      OUTCOME_TRY(param,
                  codec::cbor::decode<miner::ProveCommitAggregate::Params>(
                      message.params));
      for (const auto &sector : param.sectors) {
        on_commit(sector);
      }
    } else if (message.method == miner::ProveReplicaUpdates::Number) {
      OUTCOME_TRY(param,
                  codec::cbor::decode<miner::ProveReplicaUpdates::Params>(
                      message.params));
      for (const auto &update : param.updates) {
        on_update(update);
      }
    }
    return outcome::success();
  }

}  // namespace fc::markets::storage::chain_events
