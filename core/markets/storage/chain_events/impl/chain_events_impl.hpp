/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "markets/storage/chain_events/chain_events.hpp"

#include <mutex>

#include "api/full_node/node_api.hpp"
#include "common/logger.hpp"

namespace fc::markets::storage::chain_events {

  using adt::Channel;
  using api::FullNodeApi;
  using primitives::tipset::HeadChange;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::TipsetKey;
  using vm::message::UnsignedMessage;

  class ChainEventsImpl : public ChainEvents,
                          public std::enable_shared_from_this<ChainEventsImpl> {
   public:
    using PrecommitCb = std::function<void(outcome::result<SectorNumber>)>;

    struct Watch {
      std::multimap<DealId, PrecommitCb> precommits;
      std::multimap<SectorNumber, CommitCb> commits;
    };

    using IsDealPrecommited =
        std::function<outcome::result<boost::optional<SectorNumber>>(
            const TipsetKey &, const Address &, DealId)>;

    ChainEventsImpl(std::shared_ptr<FullNodeApi> api,
                    IsDealPrecommited is_deal_precommited);

    /**
     * Subscribe to messages
     */
    outcome::result<void> init();

    void onDealSectorCommitted(const Address &provider,
                               const DealId &deal_id,
                               CommitCb cb) override;

   private:
    bool onRead(const boost::optional<std::vector<HeadChange>> &changes);
    outcome::result<void> onMessage(const UnsignedMessage &message,
                                    const CID &cid);

    std::shared_ptr<FullNodeApi> api_;
    IsDealPrecommited is_deal_precommited_;

    /**
     * Subscription to chain head changes
     * Is alive until the channel object exists
     */
    std::shared_ptr<Channel<std::vector<HeadChange>>> channel_;

    TipsetCPtr head_;

    mutable std::mutex watched_events_mutex_;
    std::map<Address, Watch> watched_events_;

    common::Logger logger_ = common::createLogger("StorageMarketEvents");
  };
}  // namespace fc::markets::storage::chain_events
