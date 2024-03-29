/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>

#include <libp2p/basic/scheduler.hpp>

#include "network/marshalling/request_builder.hpp"

namespace fc::storage::ipfs::graphsync {
  using libp2p::basic::Scheduler;

  /// Local requests module for graphsync, manages requests made by this host
  class LocalRequests : public Subscription::Source {
   public:
    /// Context of a new request
    struct NewRequest {
      /// RAII subscription object, see libp2p
      Subscription subscription;

      /// Request ID
      RequestId request_id = 0;

      /// Serialized request body to be sent to the wire
      SharedData body;
    };

    /// LocalRequests->Graphsync feedback interface
    using CancelRequestFn = std::function<void(RequestId, SharedData)>;

    /// Ctor.
    /// \param scheduler scheduler
    /// \param cancel_fn feedback to the core component
    explicit LocalRequests(
        std::shared_ptr<Scheduler> scheduler,
        CancelRequestFn cancel_fn);

    /// Non-network part of Graphsync's makeRequest implementation.
    /// Creates a new request and NewRequest fields
    /// \param root_cid Root CID of the request
    /// \param selector IPLD selector
    /// \param extensions - protocol extension data
    /// \param callback A callback which keeps track of request progress
    /// \return request context, including serialized body
    NewRequest newRequest(const CID &root_cid,
                          gsl::span<const uint8_t> selector,
                          const std::vector<Extension> &extensions,
                          Graphsync::RequestProgressCallback callback);

    /// Creates Subscription for rejected request callback
    /// \param callback Callback which keeps track of request progress, it will
    /// receive RS_REJECTED_LOCALLY status asynchronously
    /// \return Subscription tid=ed with callback
    Subscription newRejectedRequest(
        Graphsync::RequestProgressCallback callback);

    /// Forwards responses to requests callbacks
    /// \param request_id request ID
    /// \param status response status
    /// \param extensions - data for protocol extensions
    void onResponse(RequestId request_id,
                    ResponseStatusCode status,
                    std::vector<Extension> extensions);

    /// Cancels all requests, called during Graphsync::stop()
    void cancelAll();

   private:
    /// Container that tracks all local requests
    using RequestMap = std::map<RequestId, Graphsync::RequestProgressCallback>;

    /// Subscription::Source::unsubscribe override
    void unsubscribe(uint64_t ticket) override;

    /// Calls rejected requests' callback functions in async manner
    void asyncNotifyRejectedRequests();

    /// Helper fr cancelAll(), avoids reentrancy
    static void cancelAll(RequestMap &requests);

    /// Returns the next available request id
    RequestId nextRequestId();

    /// licp2p scheduler
    std::shared_ptr<Scheduler> scheduler_;

    /// Feedback to GraphsyncImpl used to cancel requests
    CancelRequestFn cancel_fn_;

    /// All active requests
    RequestMap active_requests_;

    /// All rejected requests
    RequestMap rejected_requests_;

    /// Wire protocol requests builder (and serializer)
    RequestBuilder request_builder_;

    /// Indicates that rejected requests will be closed asynchronously in the
    /// next cycle
    bool rejected_notify_scheduled_ = false;

    /// Current request id, incremented
    RequestId current_request_id_ = 0;

    /// Current rejected id, always negative and decremented
    RequestId current_rejected_request_id_ = 0;
  };

}  // namespace fc::storage::ipfs::graphsync
