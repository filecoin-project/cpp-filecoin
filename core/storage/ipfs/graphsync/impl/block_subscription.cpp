/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "block_subscription.hpp"

#include <cassert>

#include "graphsync_impl.hpp"

namespace fc::storage::ipfs::graphsync {

  BlockSubscription::BlockSubscription(GraphsyncImpl &owner) : owner_(owner) {}

  libp2p::protocol::Subscription BlockSubscription::start(
      Graphsync::BlockCallback cb) {
    assert(cb);

    cb_ = std::move(cb);
    return Subscription(1, weak_from_this());
  }

  void BlockSubscription::forward(CID cid, common::Buffer data) {
    assert(cb_);

    cb_(std::move(cid), std::move(data));
  }

  void BlockSubscription::unsubscribe(uint64_t) {
    cb_ = Graphsync::BlockCallback{};

    // Graphsync node stops when new blocks are no more needed
    owner_.stop();
  }

}  // namespace fc::storage::ipfs::graphsync
