/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_CONNECTOR_HPP
#define CPP_FILECOIN_GRAPHSYNC_CONNECTOR_HPP

#include <functional>

#include <libp2p/host/host.hpp>

#include "../types.hpp"

namespace fc::storage::ipfs::graphsync {

  /**
   * Connects to peers and calls back to Graphsync
   */
  class Connector : public std::enable_shared_from_this<Connector> {
   public:
    using OnSessionConnected =
        std::function<void(SessionPtr session, outcome::result<void> result)>;

    Connector(std::shared_ptr<libp2p::Host> host,
              libp2p::peer::Protocol protocol_id, OnSessionConnected callback);

    void tryConnect(SessionPtr session);

   private:
    void onStreamConnected(SessionPtr session, outcome::result<StreamPtr> rstream);

    std::shared_ptr<libp2p::Host> host_;
    libp2p::peer::Protocol protocol_id_;
    OnSessionConnected callback_;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_CONNECTOR_HPP
