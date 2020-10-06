/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_EVENTS_IMPL_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_EVENTS_IMPL_HPP

#include "miner/storage_fsm/events.hpp"

#include "api/api.hpp"
#include "common/logger.hpp"
#include "miner/storage_fsm/tipset_cache.hpp"

namespace fc::mining {
  using adt::Channel;
  using api::Api;
  using primitives::tipset::HeadChange;
  using primitives::tipset::HeadChangeType;

  class EventsImpl : public Events,
                     public std::enable_shared_from_this<EventsImpl> {
   public:
    EventsImpl(std::shared_ptr<Api> api,
               std::shared_ptr<TipsetCache> tipset_cache);

    outcome::result<void> subscribeHeadChanges() override;

    void unsubscribeHeadChanges() override;

    outcome::result<void> chainAt(HeightHandler handler,
                                  RevertHandler revert_handler,
                                  EpochDuration confidence,
                                  ChainEpoch height) override;

   private:
    struct HeightHandle {
      EpochDuration confidence;
      bool called;

      HeightHandler handler;
      RevertHandler revert;
    };

    bool callback_function(const HeadChange &change);

    std::shared_ptr<Api> api_;

    /**
     * Subscription to chain head changes
     * Is alive until the channel object exists
     */
    std::shared_ptr<Channel<std::vector<HeadChange>>> channel_;

    uint64_t global_id_;

    std::unordered_map<uint64_t, HeightHandle> height_triggers_;

    std::map<ChainEpoch, std::set<uint64_t>> height_to_trigger_;  // for apply
    std::map<ChainEpoch, std::set<uint64_t>>
        message_height_to_trigger_;  // for revert

    std::mutex mutex_;

    std::shared_ptr<TipsetCache> tipset_cache_;

    common::Logger logger_ = = common::createLogger("height events");
  };

}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_EVENTS_IMPL_HPP
