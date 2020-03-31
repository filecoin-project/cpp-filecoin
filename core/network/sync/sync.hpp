/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_HPP
#define CPP_FILECOIN_SYNC_HPP

#include <functional>

#include "network/sync/sync_observer.hpp"

namespace fc::network::sync {
  class Sync {
   public:
    virtual ~Sync() = default;

    // we have no RAII due to injectors and shared_ptrs with undefined lifetime
    virtual void start() = 0;
    virtual void stop() = 0;

    // Adds new blocks observer, valid blocks received from pub-sub channel
    // are sent to observers
    virtual void listenToNewBlocks(const std::shared_ptr<BlockObserver> &o) = 0;

    // Adds new messages observer, valid messages received from pub-sub channel
    // are sent to observers
    virtual void listenToNewMessages(
        const std::shared_ptr<MessageObserver> &o) = 0;

    // Adds new sync state observer. Sync observers listen to head changes
    virtual void listenToSyncState(const std::shared_ptr<SyncObserver> &o) = 0;

    // The point is that many clients may wait for some data of a certain CID
    using LoadTicket = uint64_t;

    struct LoadResult {
      LoadTicket ticket;
      CID cid;

      // Error or data
      outcome::result<std::shared_ptr<const Buffer>> data;
    };

    using Callback = std::function<void(const LoadResult &r)>;

    // Async load. Timeout and # retries implied by config
    virtual LoadTicket load(const CID &cid, Callback cb) = 0;

    virtual void cancelLoading(LoadTicket ticket) = 0;

    // Allows for getting sync state synchronously
    virtual const SyncState &state() const = 0;

    virtual void publishBlock(const Buffer &data) = 0;
    virtual void publishMessage(const Buffer &data) = 0;
  };

}  // namespace fc::network::sync

#endif  // CPP_FILECOIN_SYNC_HPP
