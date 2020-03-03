/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_LOCAL_REQUESTS_HPP
#define CPP_FILECOIN_GRAPHSYNC_LOCAL_REQUESTS_HPP

#include <map>

#include <libp2p/protocol/common/scheduler.hpp>

#include "network/marshalling/request_builder.hpp"

namespace fc::storage::ipfs::graphsync {

  class LocalRequests : public Subscription::Source {
   public:
    struct NewRequest {
      Subscription subscription;
      RequestId request_id = 0;
      SharedData body;
    };

    using CancelRequestFn = std::function<void(RequestId, SharedData)>;

    explicit LocalRequests(
        std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
        CancelRequestFn cancel_fn);

    NewRequest newRequest(
        const CID &root_cid,
        gsl::span<const uint8_t> selector,
        bool need_metadata,
        const std::vector<CID> &dont_send_cids,
        Graphsync::RequestProgressCallback callback);

    Subscription newRejectedRequest(
        Graphsync::RequestProgressCallback callback);

    void onResponse(RequestId request_id,
                    ResponseStatusCode status,
                    ResponseMetadata metadata);

    /// Cancels all requests
    void cancelAll();

   private:
    using RequestMap = std::map<RequestId, Graphsync::RequestProgressCallback>;

    void unsubscribe(uint64_t ticket) override;

    void asyncNotifyRejectedRequests();

    static void cancelAll(RequestMap &requests);

    RequestId nextRequestId();

    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    CancelRequestFn cancel_fn_;
    RequestMap active_requests_;
    RequestMap rejected_requests_;
    RequestBuilder request_builder_;
    bool rejected_notify_scheduled_ = false;
    RequestId current_request_id_ = 0;
    RequestId current_rejected_request_id_ = 0;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_LOCAL_REQUESTS_HPP
