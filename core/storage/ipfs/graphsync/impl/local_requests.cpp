/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "local_requests.hpp"

#include <cassert>

#include "graphsync_impl.hpp"

namespace fc::storage::ipfs::graphsync {

  LocalRequests::LocalRequests(GraphsyncImpl &owner) : owner_(owner) {}

  std::pair<Subscription, int> LocalRequests::newRequest(
      Graphsync::RequestProgressCallback callback) {
    int32_t request_id = nextRequestId();
    if (request_id == 0) {
      // Will likely not get here, possible iff INT_MAX simultaneous requests
      return {Subscription(), 0};
    }
    active_requests_[request_id] = std::move(callback);
    return {Subscription(request_id, weak_from_this()), request_id};
  }

  void LocalRequests::onResponse(int request_id,
                                 ResponseStatusCode status,
                                 ResponseMetadata metadata) {
    auto it = active_requests_.find(request_id);
    if (it == active_requests_.end()) {
      // TODO log inconsistency
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

  void LocalRequests::cancelAll() {
    reentrancy_guard_ = true;
    for (const auto &[_, cb] : active_requests_) {
      cb(RS_GRAPHSYNC_STOPPED, {});
    }
    active_requests_.clear();
    reentrancy_guard_ = false;
  }

  void LocalRequests::unsubscribe(uint64_t ticket) {
    if (reentrancy_guard_) {
      return;
    }

    auto request_id = static_cast<int32_t>(ticket);
    auto it = active_requests_.find(request_id);
    if (it == active_requests_.end()) {
      return;
    }
    active_requests_.erase(it);
    owner_.cancelLocalRequest(request_id);
  }

  int32_t LocalRequests::nextRequestId() {
    if (reentrancy_guard_) {
      return 0;
    }

    int32_t id = ++current_request_id_;
    if (id > 0 && active_requests_.count(id) == 0) {
      // most likely ...
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
