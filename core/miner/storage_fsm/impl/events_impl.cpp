/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/events_impl.hpp"

namespace fc::mining {

  outcome::result<void> EventsImpl::chainAt(HeightHandler handler,
                                            RevertHandler revert_handler,
                                            ChainEpoch confidence,
                                            ChainEpoch height) {
    std::unique_lock<std::mutex> lock(mutex_);
    ChainEpoch best_height = tipset_cache_->best()->height;

    if (best_height >= height + confidence) {
      OUTCOME_TRY(tipset, tipset_cache_->getNonNull(height));

      lock.unlock();

      OUTCOME_TRY(handler(tipset, best_height));

      lock.lock();
      best_height = tipset_cache_->best()->height;
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

    message_height_to_trigger_[height].push_back(id);
    height_to_trigger_[trigger_at].push_back(id);

    return outcome::success();
  }

  void EventsImpl::callback_function(const HeadChange &change) {
    if (change.type == HeadChangeType::APPLY) {
      std::unique_lock<std::mutex> lock(mutex_);

      auto maybe_error = tipset_cache_->add(change.value);
      if (maybe_error.has_error()) {
        // TODO: log it
        return;
      }

      auto apply = [&](ChainEpoch height,
                       const Tipset &tipset) -> outcome::result<void> {
        for (const auto tid : height_to_trigger_[height]) {
          auto handler = height_triggers_[tid];
          if (handler.called) {
            return outcome::success();
          }

          auto trigger_height = height - handler.confidence;

          OUTCOME_TRY(income_tipset, tipset_cache_->getNonNull(trigger_height));

          auto handle = handler.handler;
          lock.unlock();
          auto maybe_error = handle(income_tipset, height);
          lock.lock();
          height_triggers_[tid].called = true;
          if (maybe_error.has_error()) {
            // TODO: log it
          }
        }
        return outcome::success();
      };
      auto tipset = change.value;
      maybe_error = apply(tipset.height, tipset);

      if (maybe_error.has_error()) {
        // TODO: log it
        return;
      }

      ChainEpoch sub_height = tipset.height - 1;
      while (true) {
        auto maybe_tipset_opt = tipset_cache_->get(sub_height);

        if (maybe_tipset_opt.has_error()) {
          // TODO: log it
          return;
        }

        if (maybe_tipset_opt.value()) {
          return;
        }

        maybe_error = apply(sub_height, tipset);

        if (maybe_error.has_error()) {
          // TODO: log it
          return;
        }

        sub_height--;
      }
    }

    if (change.type == HeadChangeType::REVERT) {
      // TODO: log error if h below gcconfidence
      // revert height-based triggers

      std::unique_lock<std::mutex> lock(mutex_);

      auto revert = [&](ChainEpoch height, const Tipset &tipset) {
        for (const auto tid : message_height_to_trigger_[height]) {
          auto revert_handle = height_triggers_[tid].revert;
          lock.unlock();
          auto maybe_error = revert_handle(tipset);
          lock.lock();
          height_triggers_[tid].called = false;

          if (maybe_error.has_error()) {
            // TODO: log it
          }
        }
      };

      auto tipset = change.value;

      revert(tipset.height, tipset);

      ChainEpoch sub_height = tipset.height - 1;
      while (true) {
        auto maybe_tipset_opt = tipset_cache_->get(sub_height);

        if (maybe_tipset_opt.has_error()) {
          // TODO: log it
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
        // TODO: log it
      }
      return;
    }

    // TODO: log it
  }

}  // namespace fc::mining
