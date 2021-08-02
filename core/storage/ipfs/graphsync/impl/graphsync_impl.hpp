/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <set>

#include <libp2p/basic/scheduler.hpp>

#include "network/network_fwd.hpp"

namespace fc::storage::ipfs::graphsync {
  using libp2p::basic::Scheduler;

  class LocalRequests;
  class Network;

  /// Core graphsync component. The central module
  class GraphsyncImpl final
      : public Graphsync,
        public std::enable_shared_from_this<GraphsyncImpl>,
        public PeerToGraphsyncFeedback {
   public:
    /// Ctor.
    /// \param host libp2p host object
    /// \param scheduler libp2p scheduler
    GraphsyncImpl(std::shared_ptr<libp2p::Host> host,
                  std::shared_ptr<Scheduler> scheduler);

    ~GraphsyncImpl() override;

    /// Callback from LocalRequests module. Cancels a request made by this host
    /// \param request_id request ID
    /// \param body request wire protocol data
    void cancelLocalRequest(RequestId request_id, SharedData body);

    // Graphsync interface overrides
    DataConnection subscribe(std::function<OnDataReceived> handler) override;
    void setDefaultRequestHandler(
        std::function<RequestHandler> handler) override;
    void setRequestHandler(std::function<RequestHandler> handler,
                           std::string extension_name) override;
    void postResponse(const FullRequestId &id,
                      const Response &response) override;
    void start() override;
    void stop() override;
    Subscription makeRequest(
        const libp2p::peer::PeerId &peer,
        boost::optional<libp2p::multi::Multiaddress> address,
        const CID &root_cid,
        gsl::span<const uint8_t> selector,
        const std::vector<Extension> &extensions,
        RequestProgressCallback callback) override;

    // PeerToGraphsyncFeedback interface overrides
    void onResponse(const PeerId &peer,
                    RequestId request_id,
                    ResponseStatusCode status,
                    std::vector<Extension> extensions) override;
    void onDataBlock(const PeerId &from, Data data) override;
    void onRemoteRequest(const PeerId &from, Message::Request request) override;

    /// NVI for stop()
    void doStop();

    /// Scheduler for libp2p
    std::shared_ptr<Scheduler> scheduler_;

    /// Network module
    std::shared_ptr<Network> network_;

    /// Local requests handling module
    std::shared_ptr<LocalRequests> local_requests_;

    std::function<RequestHandler> default_request_handler_;
    std::map<std::string, std::function<RequestHandler>> request_handlers_;

    /// Subscriptions to data to blocks
    boost::signals2::signal<OnDataReceived> data_signal_;

    /// Flag, indicates that instance is started
    bool started_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync
