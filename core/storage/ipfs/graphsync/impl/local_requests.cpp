/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>

#include "local_requests.hpp"

namespace fc::storage::ipfs::graphsync {

  LocalRequests::LocalRequests(
      std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
      CancelRequestFn cancel_fn)
      : scheduler_(std::move(scheduler)), cancel_fn_(std::move(cancel_fn)) {
    assert(scheduler_);
    assert(cancel_fn_);
  }

  LocalRequests::NewRequest LocalRequests::newRequest(
      const CID &root_cid,
      gsl::span<const uint8_t> selector,
      bool need_metadata,
      const std::vector<CID> &dont_send_cids,
      Graphsync::RequestProgressCallback callback) {
    NewRequest ctx;
    ctx.request_id = nextRequestId();
    if (ctx.request_id == 0) {
      // Will likely not get here, possible iff INT_MAX simultaneous requests
      logger()->error("{}: request ids exhausted", __FUNCTION__);
      ctx.subscription = newRejectedRequest(std::move(callback));
      ctx.request_id = current_rejected_request_id_;
      return ctx;
    }

    request_builder_.addRequest(
        ctx.request_id, root_cid, selector, need_metadata, dont_send_cids);
    auto serialize_res = request_builder_.serialize();
    request_builder_.clear();
    if (!serialize_res) {
      logger()->error("{}: serialize failed", __FUNCTION__);
      ctx.subscription = newRejectedRequest(std::move(callback));
      ctx.request_id = current_rejected_request_id_;
      return ctx;
    }

    ctx.subscription = Subscription(ctx.request_id, weak_from_this());
    ctx.body = std::move(serialize_res.value());
    active_requests_[ctx.request_id] = std::move(callback);

    logger()->trace("{}: id={}", __FUNCTION__, ctx.request_id);

    return ctx;
  }

  Subscription LocalRequests::newRejectedRequest(
      Graphsync::RequestProgressCallback callback) {
    RequestId request_id = --current_rejected_request_id_;
    if (request_id == 0) {
      logger()->error("{}: rejected request ids exhausted", __FUNCTION__);
      // virtually impossible
      return Subscription();
    }
    rejected_requests_[request_id] = std::move(callback);
    asyncNotifyRejectedRequests();
    return Subscription(request_id, weak_from_this());
  }

  void LocalRequests::onResponse(int request_id,
                                 ResponseStatusCode status,
                                 ResponseMetadata metadata) {
    auto it = active_requests_.find(request_id);
    if (it == active_requests_.end()) {
      logger()->error(
          "{}: cannot find request, id={}", __FUNCTION__, request_id);
      return;
    }
    if (isTerminal(status)) {
      auto cb = std::move(it->second);
      active_requests_.erase(it);
      cb(status, std::move(metadata));
    } else {
      // make copy, reentrancy is allowed here
      auto cb = it->second;
      cb(status, std::move(metadata));
    }
  }

  void LocalRequests::cancelAll(LocalRequests::RequestMap &requests) {
    if (requests.empty()) {
      return;
    }
    RequestMap m;
    std::swap(m, requests);
    for (const auto &[_, cb] : m) {
      cb(RS_REJECTED_LOCALLY, {});
    }
  }

  void LocalRequests::asyncNotifyRejectedRequests() {
    if (rejected_notify_scheduled_) {
      return;
    }

    scheduler_
        ->schedule([wptr = weak_from_this(), this]() {
          if (!wptr.expired()) {
            cancelAll(rejected_requests_);
            if (rejected_requests_.empty()) {
              current_rejected_request_id_ = 0;
            }
          }
        })
        .detach();
  }

  void LocalRequests::cancelAll() {
    cancelAll(active_requests_);
    cancelAll(rejected_requests_);
  }

  void LocalRequests::unsubscribe(uint64_t ticket) {
    auto request_id = static_cast<RequestId>(ticket);

    if (request_id < 0) {
      // this is rejected request
      rejected_requests_.erase(request_id);
      return;
    }

    auto it = active_requests_.find(request_id);
    if (it == active_requests_.end()) {
      return;
    }
    active_requests_.erase(it);

    request_builder_.addCancelRequest(request_id);
    auto serialize_res = request_builder_.serialize();
    request_builder_.clear();

    cancel_fn_(request_id,
               serialize_res ? serialize_res.value() : SharedData{});
  }

  RequestId LocalRequests::nextRequestId() {
    RequestId id = ++current_request_id_;
    if (id > 0 && active_requests_.count(id) == 0) {
      return id;
    }
    // We prepare for long uptimes,
    // the graphsync wire protocol does not (int32 there)
    id = 1;
    while (active_requests_.count(id) != 0) {
      if (++id == 0) {
        // almost impossible
        return 0;
      }
    }
    current_request_id_ = id;
    return id;
  }

}  // namespace fc::storage::ipfs::graphsync
