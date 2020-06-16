/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_EVENTS_IMPL_EVENTS_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_EVENTS_IMPL_EVENTS_IMPL_HPP

#include "markets/storage/events/events.hpp"

#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <shared_mutex>
#include "api/api.hpp"
#include "host/context/host_context.hpp"
#include "storage/mpool/mpool.hpp"

namespace fc::markets::storage::events {

  using adt::Channel;
  using api::Api;
  using api::Chan;
  using fc::storage::mpool::MpoolUpdate;
  using host::HostContext;
  using libp2p::protocol::Scheduler;

  const int kTicks = 50;

  class EventsImpl : public Events,
                     public std::enable_shared_from_this<EventsImpl> {
   public:
    explicit EventsImpl(std::shared_ptr<Api> api,
                        std::shared_ptr<HostContext> context);
    ~EventsImpl() override;

    outcome::result<void> run() override;
    void stop() override;
    std::shared_ptr<PromiseResult> onDealSectorCommitted(
        const Address &provider, const DealId &deal_id) override;

   private:
    void onTimer();

    bool running_;
    std::shared_ptr<Api> api_;
    std::shared_ptr<HostContext> host_context_;
    std::shared_ptr<Scheduler> scheduler_;
    Scheduler::Handle scheduler_handle_;
    std::shared_ptr<Channel<MpoolUpdate>> channel_;

    mutable std::shared_mutex watched_events_mutex_;
    std::vector<EventWatch> watched_events_;

    common::Logger logger_ = common::createLogger("StorageMarketEvents");
  };
}  // namespace fc::markets::storage::events

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_EVENTS_IMPL_EVENTS_IMPL_HPP
