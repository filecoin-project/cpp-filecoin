/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/events.hpp"

#include "api/full_node/node_api.hpp"
#include "common/logger.hpp"
#include "miner/storage_fsm/tipset_cache.hpp"

namespace fc::mining {
  using adt::Channel;
  using api::FullNodeApi;
  using primitives::tipset::HeadChange;
  using primitives::tipset::HeadChangeType;

  class EventsImpl : public Events,
                     public std::enable_shared_from_this<EventsImpl> {
   public:
    EventsImpl(const EventsImpl &) = delete;
    EventsImpl(EventsImpl &&) = delete;
    ~EventsImpl() override;

    EventsImpl &operator=(const EventsImpl &) = delete;
    EventsImpl &operator=(EventsImpl &&) = delete;

    static outcome::result<std::shared_ptr<EventsImpl>> createEvents(
        const std::shared_ptr<FullNodeApi> &api,
        std::shared_ptr<TipsetCache> tipset_cache);

    outcome::result<void> chainAt(HeightHandler handler,
                                  RevertHandler revert_handler,
                                  EpochDuration confidence,
                                  ChainEpoch height) override;

   private:
    explicit EventsImpl(std::shared_ptr<TipsetCache> tipset_cache);

    struct HeightHandle {
      EpochDuration confidence;
      bool called;

      HeightHandler handler;
      RevertHandler revert;
    };

    bool callback_function(const HeadChange &change);

    std::shared_ptr<FullNodeApi> api_;

    /**
     * Subscription to chain head changes
     * Is alive until the channel object exists
     */
    std::shared_ptr<Channel<std::vector<HeadChange>>> channel_;

    uint64_t global_id_;

    // TODO(turuslan): FIL-420 check cache memory usage
    std::unordered_map<uint64_t, HeightHandle> height_triggers_;

    // TODO(turuslan): FIL-420 check cache memory usage
    std::map<ChainEpoch, std::set<uint64_t>> height_to_trigger_;  // for apply
    // TODO(turuslan): FIL-420 check cache memory usage
    std::map<ChainEpoch, std::set<uint64_t>>
        message_height_to_trigger_;  // for revert

    std::mutex mutex_;

    std::shared_ptr<TipsetCache> tipset_cache_;

    common::Logger logger_ = common::createLogger("height events");
  };

}  // namespace fc::mining
