/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/events_impl.hpp"

#include <set>

namespace fc::mining {

  EventsImpl::EventsImpl(std::shared_ptr<TipsetCache> tipset_cache)
      : global_id_(0), tipset_cache_(std::move(tipset_cache)) {}

  outcome::result<std::shared_ptr<EventsImpl>> EventsImpl::createEvents(
      const std::shared_ptr<FullNodeApi> &api,
      std::shared_ptr<TipsetCache> tipset_cache) {
    struct make_unique_enabler : public EventsImpl {
      make_unique_enabler(std::shared_ptr<TipsetCache> tipset_cache)
          : EventsImpl{std::move(tipset_cache)} {};
    };

    std::shared_ptr<EventsImpl> events =
        std::make_unique<make_unique_enabler>(std::move(tipset_cache));

    OUTCOME_TRY(chan, api->ChainNotify());
    events->channel_ = chan.channel;
    events->channel_->read(
        [self_weak{events->weak_from_this()}](
            const boost::optional<std::vector<HeadChange>> &changes) -> bool {
          if (auto self = self_weak.lock()) {
            if (changes) {
              for (const auto &change : changes.value()) {
                if (!self->callback_function(change)) return false;
              }
            }
            return true;
          }
          return false;
        });

    return events;
  }

  outcome::result<void> EventsImpl::chainAt(HeightHandler handler,
                                            RevertHandler revert_handler,
                                            EpochDuration confidence,
                                            ChainEpoch height) {
    std::unique_lock<std::mutex> lock(mutex_);
    OUTCOME_TRY(best_tipset, tipset_cache_->best());

    ChainEpoch best_height = best_tipset.height();

    if (best_height >= height + confidence) {
      OUTCOME_TRY(tipset, tipset_cache_->getNonNull(height));

      lock.unlock();

      OUTCOME_TRY(handler(tipset, best_height));

      lock.lock();
      OUTCOME_TRYA(best_tipset, tipset_cache_->best());

      best_height = best_tipset.height();

      if (best_height >= height + confidence + kGlobalChainConfidence) {
        return outcome::success();
      }
    }

    ChainEpoch trigger_at = height + confidence;

    // TODO: maybe overflow
    uint64_t id = global_id_++;

    height_triggers_[id] = HeightHandle{
        .confidence = confidence,
        .called = false,
        .handler = handler,
        .revert = revert_handler,
    };

    message_height_to_trigger_[height].insert(id);
    height_to_trigger_[trigger_at].insert(id);

    return outcome::success();
  }

  bool EventsImpl::callback_function(const HeadChange &change) {
    if (change.type == HeadChangeType::APPLY) {
      std::unique_lock<std::mutex> lock(mutex_);

      auto maybe_error = tipset_cache_->add(*change.value);
      if (maybe_error.has_error()) {
        logger_->error("Adding tipset into cache failed: {}",
                       maybe_error.error().message());
        return false;
      }

      auto apply = [&](ChainEpoch height) -> outcome::result<void> {
        for (const auto tid : height_to_trigger_[height]) {
          auto &handler{height_triggers_.at(tid)};
          if (handler.called) {
            return outcome::success();
          }

          auto trigger_height = height - handler.confidence;

          OUTCOME_TRY(income_tipset, tipset_cache_->getNonNull(trigger_height));

          auto &handle{handler.handler};
          lock.unlock();
          auto maybe_error = handle(income_tipset, height);
          lock.lock();
          height_triggers_[tid].called = true;
          if (maybe_error.has_error()) {
            logger_->error("Height handler is failed: {}",
                           maybe_error.error().message());
          }
        }
        return outcome::success();
      };
      auto tipset = change.value;
      maybe_error = apply(tipset->height());

      if (maybe_error.has_error()) {
        logger_->error("Applying tipset failed: {}",
                       maybe_error.error().message());
        return false;
      }

      ChainEpoch sub_height = tipset->height() - 1;
      while (true) {
        auto maybe_tipset_opt = tipset_cache_->get(sub_height);

        if (maybe_tipset_opt.has_error()) {
          logger_->error("Getting tipset from cache failed: {}",
                         maybe_tipset_opt.error().message());
          return false;
        }

        if (maybe_tipset_opt.value()) {
          return true;
        }

        maybe_error = apply(sub_height);

        if (maybe_error.has_error()) {
          logger_->error("Applying tipset failed: {}",
                         maybe_error.error().message());
          return false;
        }

        sub_height--;
      }

      return true;
    }

    if (change.type == HeadChangeType::REVERT) {
      std::unique_lock<std::mutex> lock(mutex_);

      // TODO (ortyomka):[FIL-371] log error if h below gcconfidence
      // revert height-based triggers

      auto revert = [&](ChainEpoch height, const Tipset &tipset) {
        for (const auto tid : message_height_to_trigger_[height]) {
          auto &revert_handle{height_triggers_[tid].revert};
          lock.unlock();
          auto maybe_error = revert_handle(tipset);
          lock.lock();
          height_triggers_[tid].called = false;

          if (maybe_error.has_error()) {
            logger_->error("Revert handler is failed: {}",
                           maybe_error.error().message());
          }
        }
      };

      auto tipset = change.value;

      revert(tipset->height(), *tipset);

      ChainEpoch sub_height = tipset->height() - 1;
      while (true) {
        auto maybe_tipset_opt = tipset_cache_->get(sub_height);

        if (maybe_tipset_opt.has_error()) {
          logger_->error("Getting tipset from cache failed: {}",
                         maybe_tipset_opt.error().message());
          return false;
        }

        if (maybe_tipset_opt.value()) {
          break;
        }

        revert(sub_height, *tipset);
        sub_height--;
      }

      auto maybe_error = tipset_cache_->revert(*tipset);
      if (maybe_error.has_error()) {
        logger_->error("Reverting tipset failed: {}",
                       maybe_error.error().message());
        return false;
      }
      return true;
    }

    return true;
  }

  EventsImpl::~EventsImpl() {
    channel_ = nullptr;
  }

}  // namespace fc::mining
