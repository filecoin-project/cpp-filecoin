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
  using primitives::tipset::HeadChange;

  class EventsImpl : public Events,
                     public std::enable_shared_from_this<EventsImpl> {
   public:
    EventsImpl(std::shared_ptr<Api> api, IpldPtr ipld);

    /**
     * Subscribe to messages
     */
    outcome::result<void> init();

    std::shared_ptr<PromiseResult> onDealSectorCommitted(
        const Address &provider, const DealId &deal_id) override;

   private:
    bool onRead(const boost::optional<std::vector<HeadChange>> &changes);
    outcome::result<void> onMessage(bool bls, const CID &message_cid);

    std::shared_ptr<Api> api_;
    IpldPtr ipld_;
    mutable std::shared_mutex watched_events_mutex_;
    std::vector<EventWatch> watched_events_;

    common::Logger logger_ = common::createLogger("StorageMarketEvents");
  };
}  // namespace fc::markets::storage::events

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_EVENTS_IMPL_EVENTS_IMPL_HPP
