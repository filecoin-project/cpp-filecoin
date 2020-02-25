/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_BLOCKSUBSCRIPTION_HPP
#define CPP_FILECOIN_GRAPHSYNC_BLOCKSUBSCRIPTION_HPP

#include "common.hpp"

namespace fc::storage::ipfs::graphsync {

  class GraphsyncImpl;

  /// Subscription to blocks from the network
  class BlockSubscription : public libp2p::protocol::Subscription::Source {
   public:
    explicit BlockSubscription(GraphsyncImpl& owner);

    libp2p::protocol::Subscription start(Graphsync::BlockCallback cb);

    void forward(CID cid, common::Buffer data);

   private:
    void unsubscribe(uint64_t) override;

    /// Owner, get notified when requests unsubscribe
    GraphsyncImpl& owner_;

    /// The only subscription to block (at the moment)
    Graphsync::BlockCallback cb_;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_BLOCKSUBSCRIPTION_HPP
