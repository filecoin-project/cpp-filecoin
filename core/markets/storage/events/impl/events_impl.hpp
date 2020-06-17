/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_EVENTS_IMPL_EVENTS_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_EVENTS_IMPL_EVENTS_IMPL_HPP

#include "markets/storage/events/events.hpp"

#include <shared_mutex>
#include "api/api.hpp"
#include "storage/mpool/mpool.hpp"

namespace fc::markets::storage::events {

  using adt::Channel;
  using api::Api;
  using api::Chan;
  using fc::storage::mpool::MpoolUpdate;

  class EventsImpl : public Events,
                     public std::enable_shared_from_this<EventsImpl> {
   public:
    explicit EventsImpl(std::shared_ptr<Api> api);

    /**
     * Subscribe to messages
     */
    outcome::result<void> init();

    std::shared_ptr<PromiseResult> onDealSectorCommitted(
        const Address &provider, const DealId &deal_id) override;

   private:
    bool onRead(const boost::optional<MpoolUpdate> &update);

    std::shared_ptr<Api> api_;
    mutable std::shared_mutex watched_events_mutex_;
    std::vector<EventWatch> watched_events_;

    common::Logger logger_ = common::createLogger("StorageMarketEvents");
  };
}  // namespace fc::markets::storage::events

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_EVENTS_IMPL_EVENTS_IMPL_HPP
