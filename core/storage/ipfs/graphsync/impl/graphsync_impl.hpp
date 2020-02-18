/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_IMPL_HPP
#define CPP_FILECOIN_GRAPHSYNC_IMPL_HPP

#include <set>

#include "message.hpp"
#include "types.hpp"

#include "storage/ipfs/graphsync/graphsync.hpp"
#include "block_subscription.hpp"

namespace libp2p {
  class Host;
}

namespace fc::storage::ipfs::graphsync {

  class Server;
  class Connector;
  class BlockSubscription;

  class GraphsyncImpl : public Graphsync {
   public:
    GraphsyncImpl(std::shared_ptr<libp2p::Host> host);

    // using RequestId = int;

    /// Stops server and all requests
    void stop();

    // RequestId makeRequest

   private:
    Subscription start(std::shared_ptr<MerkleDagBridge> dag,
                       BlockCallback callback) override;

    Subscription makeRequest(
        libp2p::peer::PeerId peer,
        boost::optional<libp2p::multi::Multiaddress> address,
        gsl::span<const uint8_t> root_cid,
        gsl::span<const uint8_t> selector,
        bool need_metadata,
        std::unordered_set<common::Buffer> dont_send_cids) override;

    void onResponse(Message& msg);

    void onSessionAccepted(SessionPtr session, Message msg);

    void onSessionConnected(SessionPtr session, outcome::result<void> result);

    std::shared_ptr<libp2p::Host> host_;

    std::shared_ptr<Server> server_;

    std::set<SessionPtr> connecting_sessions_;

    /// Outbound session ids are odd
    uint64_t current_session_id_ = 1;

    int current_request_id_ = 0;

    std::shared_ptr<BlockSubscription> block_subscription_;
    std::shared_ptr<MerkleDagBridge> dag_;

    bool started_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_IMPL_HPP
