/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_EVENTS_IMPL_EVENTS_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_EVENTS_IMPL_EVENTS_IMPL_HPP

#include "markets/storage/chain_events/chain_events.hpp"

#include <shared_mutex>

#include "api/api.hpp"
#include "common/logger.hpp"

namespace fc::markets::storage::chain_events {

  using adt::Channel;
  using api::Api;
  using primitives::tipset::HeadChange;
  using vm::message::UnsignedMessage;

  class ChainEventsImpl : public ChainEvents,
                          public std::enable_shared_from_this<ChainEventsImpl> {
   public:
    ChainEventsImpl(std::shared_ptr<Api> api);

    /**
     * Subscribe to messages
     */
    outcome::result<void> init();

    void onDealSectorCommitted(const Address &provider,
                               const DealId &deal_id,
                               Cb cb) override;

   private:
    bool onRead(const boost::optional<std::vector<HeadChange>> &changes);
    outcome::result<void> onMessage(const UnsignedMessage &message,
                                    const CID &cid);

    std::shared_ptr<Api> api_;

    /**
     * Subscription to chain head changes
     * Is alive until the channel object exists
     */
    std::shared_ptr<Channel<std::vector<HeadChange>>> channel_;

    mutable std::shared_mutex watched_events_mutex_;
    std::vector<EventWatch> watched_events_;

    common::Logger logger_ = common::createLogger("StorageMarketEvents");
  };
}  // namespace fc::markets::storage::chain_events

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_EVENTS_IMPL_EVENTS_IMPL_HPP
