/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_IMPL_HPP
#define CPP_FILECOIN_GRAPHSYNC_IMPL_HPP

#include <set>

#include <libp2p/protocol/common/scheduler.hpp>

#include "network/network_fwd.hpp"

namespace libp2p {
  class Host;
}

namespace fc::storage::ipfs::graphsync {

  class LocalRequests;
  class Network;

  class GraphsyncImpl : public Graphsync,
                        public std::enable_shared_from_this<GraphsyncImpl>,
                        public PeerToGraphsyncFeedback {
   public:
    GraphsyncImpl(std::shared_ptr<libp2p::Host> host,
                  std::shared_ptr<libp2p::protocol::Scheduler> scheduler);

    ~GraphsyncImpl() override;

   private:
    void cancelLocalRequest(RequestId request_id, SharedData body);

    void start(std::shared_ptr<MerkleDagBridge> dag,
               BlockCallback callback) override;

    void stop() override;

    Subscription makeRequest(
        const libp2p::peer::PeerId &peer,
        boost::optional<libp2p::multi::Multiaddress> address,
        const CID &root_cid,
        gsl::span<const uint8_t> selector,
        bool need_metadata,
        const std::vector<CID> &dont_send_cids,
        RequestProgressCallback callback) override;

    void onResponse(const PeerId &peer,
                    RequestId request_id,
                    ResponseStatusCode status,
                    ResponseMetadata metadata) override;

    void onBlock(const PeerId &from, CID cid, common::Buffer data) override;

    void onRemoteRequest(const PeerId &from,
                         Message::Request request) override;

    void doStop();

    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;

    std::shared_ptr<Network> network_;

    std::shared_ptr<LocalRequests> local_requests_;

    std::shared_ptr<MerkleDagBridge> dag_;

    /// The only subscription to block (at the moment)
    Graphsync::BlockCallback block_cb_;

    bool started_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_IMPL_HPP
