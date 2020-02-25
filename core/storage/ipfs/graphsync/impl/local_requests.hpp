/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_LOCAL_REQUESTS_HPP
#define CPP_FILECOIN_GRAPHSYNC_LOCAL_REQUESTS_HPP

#include <map>

#include "common.hpp"

namespace fc::storage::ipfs::graphsync {

  class GraphsyncImpl;

  class LocalRequests : public Subscription::Source {
   public:
    explicit LocalRequests(GraphsyncImpl &owner);

    std::pair<Subscription, int> newRequest(
        Graphsync::RequestProgressCallback callback);

    void onResponse(int request_id,
                    ResponseStatusCode status,
                    ResponseMetadata metadata);

    /// Cancels all active requests
    void cancelAll();

   private:
    void unsubscribe(uint64_t ticket) override;

    int32_t nextRequestId();

    GraphsyncImpl &owner_;
    std::map<int, Graphsync::RequestProgressCallback> active_requests_;
    int32_t current_request_id_ = 0;
    bool reentrancy_guard_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_LOCAL_REQUESTS_HPP
