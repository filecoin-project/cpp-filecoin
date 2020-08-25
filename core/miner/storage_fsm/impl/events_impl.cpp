/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/events_impl.hpp"

#include <set>

namespace fc::mining {

  outcome::result<void> EventsImpl::chainAt(HeightHandler handler,
                                            RevertHandler revert_handler,
                                            EpochDuration confidence,
                                            ChainEpoch height) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto best_tipset = tipset_cache_->best();

    if (!best_tipset) {
      return EventsError::kNotFoundTipset;
    }

    ChainEpoch best_height = best_tipset->height;

    if (best_height >= height + confidence) {
      OUTCOME_TRY(tipset, tipset_cache_->getNonNull(height));

      lock.unlock();

      OUTCOME_TRY(handler(tipset, best_height));

      lock.lock();
      best_tipset = tipset_cache_->best();

      if (!best_tipset) {
        return EventsError::kNotFoundTipset;
      }
      best_height = best_tipset->height;
    }

    if (best_height >= height + confidence + kGlobalChainConfidence) {
      return outcome::success();
    }

    ChainEpoch trigger_at = height + confidence;

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

  void EventsImpl::callback_function(const HeadChange &change) {
    if (change.type == HeadChangeType::APPLY) {
      std::unique_lock<std::mutex> lock(mutex_);

      auto maybe_error = tipset_cache_->add(change.value);
      if (maybe_error.has_error()) {
        logger_->error("Adding tipset into cache failed: {}",
                       maybe_error.error().message());
        return;
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
      maybe_error = apply(tipset.height);

      if (maybe_error.has_error()) {
        logger_->error("Applying tipset failed: {}",
                       maybe_error.error().message());
        return;
      }

      ChainEpoch sub_height = tipset.height - 1;
      while (true) {
        auto maybe_tipset_opt = tipset_cache_->get(sub_height);

        if (maybe_tipset_opt.has_error()) {
          logger_->error("Getting tipset from cache failed: {}",
                         maybe_tipset_opt.error().message());
          return;
        }

        if (maybe_tipset_opt.value()) {
          return;
        }

        maybe_error = apply(sub_height);

        if (maybe_error.has_error()) {
          logger_->error("Applying tipset failed: {}",
                         maybe_error.error().message());
          return;
        }

        sub_height--;
      }
    }

    if (change.type == HeadChangeType::REVERT) {
      std::unique_lock<std::mutex> lock(mutex_);

      auto best_tipset = tipset_cache_->best();

      if (!best_tipset) {
        logger_->error("Cache is empty");
        return;
      }

      ChainEpoch best_height = best_tipset->height;

      auto revert = [&](ChainEpoch height, const Tipset &tipset) {
        if (best_height >= height + kGlobalChainConfidence) {
          logger_->warn("Tipset is deprecated");
          for (const auto tid : message_height_to_trigger_[height]) {
            auto &trigger{height_triggers_.at(tid)};
            auto id = height + trigger.confidence;
            auto maybe_error = trigger.revert(tipset);
            if (maybe_error.has_error()) {
              logger_->error("Revert tipset (height {}) is failed: {}",
                             tipset.height,
                             maybe_error.has_error());
            }
            height_to_trigger_[id].erase(tid);
            height_triggers_.erase(tid);
          }
          message_height_to_trigger_.erase(height);
          return;
        }

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

          auto best_tipset = tipset_cache_->best();

          if (!best_tipset) {
            logger_->error("Cache is empty");
            return;
          }

          best_height = best_tipset->height;
        }
      };

      auto tipset = change.value;

      revert(tipset.height, tipset);

      ChainEpoch sub_height = tipset.height - 1;
      while (true) {
        auto maybe_tipset_opt = tipset_cache_->get(sub_height);

        if (maybe_tipset_opt.has_error()) {
          logger_->error("Getting tipset from cache failed: {}",
                         maybe_tipset_opt.error().message());
          break;
        }

        if (maybe_tipset_opt.value()) {
          break;
        }

        revert(sub_height, tipset);
        sub_height--;
      }

      auto maybe_error = tipset_cache_->revert(tipset);
      if (maybe_error.has_error()) {
        logger_->error("Reverting tipset failed: {}",
                       maybe_error.error().message());
      }
      return;
    }

    logger_->warn("Unexpected head change notification type");
  }

  EventsImpl::EventsImpl(const std::shared_ptr<ChainStore> &chain_store,
                         std::shared_ptr<TipsetCache> tipset_cache)
      : global_id_(0), tipset_cache_(std::move(tipset_cache)) {
    connection_ = chain_store->subscribeHeadChanges(
        std::bind(&EventsImpl::callback_function, this, std::placeholders::_1));
  }

}  // namespace fc::mining

OUTCOME_CPP_DEFINE_CATEGORY(fc::mining, EventsError, e) {
  using fc::mining::EventsError;
  switch (e) {
    case (EventsError::kNotFoundTipset):
      return "Events: not found tipset";
    default:
      return "Events: unknown error";
  }
}
