/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_IMPL_HPP
#define CPP_FILECOIN_GRAPHSYNC_IMPL_HPP

#include <set>

#include "message.hpp"
#include "block_subscription.hpp"
#include "network/network.hpp"
#include "network/marshalling/request_builder.hpp"

namespace libp2p {
  class Host;
}

namespace fc::storage::ipfs::graphsync {

  class LocalRequests;
  class BlockSubscription;

  class GraphsyncImpl : public Graphsync,
                        public std::enable_shared_from_this<GraphsyncImpl>,
                        public NetworkEvents {
   public:
    GraphsyncImpl(std::shared_ptr<libp2p::Host> host);

    /// Stops server and all requests.
    void stop();

    void cancelLocalRequest(int request_id);

   private:
    Subscription start(std::shared_ptr<MerkleDagBridge> dag,
                       BlockCallback callback) override;

    Subscription makeRequest(
        const libp2p::peer::PeerId &peer,
        boost::optional<libp2p::multi::Multiaddress> address,
        gsl::span<const uint8_t> root_cid,
        gsl::span<const uint8_t> selector,
        bool need_metadata,
        const std::vector<CID> &dont_send_cids,
        RequestProgressCallback callback) override;

    void onResponse(const PeerId &peer,
                    int request_id,
                    ResponseStatusCode status,
                    ResponseMetadata metadata) override;

    void onBlock(const PeerId &from, CID cid, common::Buffer data) override;

    void onRemoteRequest(const PeerId &from,
                         uint64_t tag,
                         Message::Request request) override;

    std::shared_ptr<libp2p::Host> host_;

    std::shared_ptr<Network> network_;

    std::shared_ptr<LocalRequests> local_requests_;

    std::shared_ptr<BlockSubscription> block_subscription_;

    std::shared_ptr<MerkleDagBridge> dag_;

    RequestBuilder request_builder_;

    bool started_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_IMPL_HPP
