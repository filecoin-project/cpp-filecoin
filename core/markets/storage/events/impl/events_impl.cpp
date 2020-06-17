/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/events/impl/events_impl.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"

namespace fc::markets::storage::events {
  using vm::actor::builtin::miner::PreCommitSector;
  using vm::actor::builtin::miner::ProveCommitSector;
  using vm::actor::builtin::miner::SectorPreCommitInfo;
  using PromiseResult = Events::PromiseResult;

  EventsImpl::EventsImpl(std::shared_ptr<Api> api) : api_{std::move(api)} {}

  outcome::result<void> EventsImpl::init() {
    OUTCOME_TRY(chan, api_->MpoolSub());
    chan.channel->read([self{shared_from_this()}, channel{chan.channel}](
                           boost::optional<MpoolUpdate> update) -> bool {
      return self->onRead(update);
    });
    return outcome::success();
  }

  std::shared_ptr<PromiseResult> EventsImpl::onDealSectorCommitted(
      const Address &provider, const DealId &deal_id) {
    auto result = std::make_shared<PromiseResult>();
    std::unique_lock lock(watched_events_mutex_);
    watched_events_.emplace_back(EventWatch{.provider = provider,
                                            .deal_id = deal_id,
                                            .sector_number = boost::none,
                                            .result = result});
    return result;
  }

  /**
   * Actually sector commit consists of 2 method calls:
   *  1) PreCommitSector with desired provider address and deal id. Parameters
   * contain sector number used in the next call
   *  2) ProveCommitSector with desired provider address and sector number
   */
  bool EventsImpl::onRead(const boost::optional<MpoolUpdate> &update) {
    std::vector<EventWatch>::iterator watch_it;
    boost::optional<SectorNumber> update_sector_number;
    bool prove_sector_committed = false;
    {
      std::shared_lock lock(watched_events_mutex_);
      for (watch_it = watched_events_.begin();
           watch_it != watched_events_.end();
           ++watch_it) {
        // message committed iff removed
        if (update.has_value() && update->type == MpoolUpdate::Type::REMOVE
            && update->message.message.to == watch_it->provider) {
          auto message = update->message.message;
          if (message.method == PreCommitSector::Number) {
            auto maybe_pre_commit_info =
                codec::cbor::decode<SectorPreCommitInfo>(message.params);
            if (maybe_pre_commit_info.has_error()) {
              logger_->error("Decode SectorPreCommitInfo params error "
                             + maybe_pre_commit_info.error().message());
              break;
            }

            auto deal_ids = maybe_pre_commit_info.value().deal_ids;
            if (std::find(deal_ids.begin(), deal_ids.end(), watch_it->deal_id)
                != deal_ids.end()) {
              update_sector_number = maybe_pre_commit_info.value().sector;
              break;
            }
          } else if (message.method == ProveCommitSector::Number) {
            auto maybe_prove_commit_params =
                codec::cbor::decode<ProveCommitSector::Params>(message.params);
            if (maybe_prove_commit_params.has_error()) {
              logger_->error("Decode ProveCommitSector params error "
                             + maybe_prove_commit_params.error().message());
              break;
            }

            if (watch_it->sector_number
                && maybe_prove_commit_params.value().sector
                       == watch_it->sector_number) {
              prove_sector_committed = true;
              break;
            }
          }
        }
      }
    }

    if (watch_it != watched_events_.end()) {
      std::unique_lock lock(watched_events_mutex_);
      if (update_sector_number) {
        watch_it->sector_number = update_sector_number;
      }

      if (prove_sector_committed) {
        watch_it->result->set_value(outcome::success());
        watched_events_.erase(watch_it);
      }
    }
    return true;
  }

}  // namespace fc::markets::storage::events
