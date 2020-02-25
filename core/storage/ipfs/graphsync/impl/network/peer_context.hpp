/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_PEER_CONTEXT_HPP
#define CPP_FILECOIN_GRAPHSYNC_PEER_CONTEXT_HPP

#include <map>
#include <set>

#include "network_fwd.hpp"
#include "storage/ipfs/graphsync/impl/network/marshalling/response_builder.hpp"

namespace fc::storage::ipfs::graphsync {

  class Endpoint;
  using EndpointPtr = std::shared_ptr<Endpoint>;

  struct PeerContext {
    ~PeerContext() = default;
    PeerContext(PeerContext &&) = delete;
    PeerContext(const PeerContext &) = delete;
    PeerContext &operator=(const PeerContext &) = delete;
    PeerContext &operator=(PeerContext &&) = delete;

    /// The only ctor, since PeerContext is to be used in shared_ptr only
    explicit PeerContext(PeerId peer_id);

    /// Remote peer
    const PeerId peer;

    /// String representation for loggers and debug purposes
    const std::string str;

    boost::optional<libp2p::multi::Multiaddress> connect_to;
    bool is_connecting = false;

    EndpointPtr connected_endpoint;

    struct ResponseCtx {
      EndpointPtr endpoint;
      ResponseBuilder response_builder;
    };

    std::map<uint64_t, ResponseCtx> accepted_endpoints;

    uint64_t current_endpoint_tag = 1;

    std::set<int> local_request_ids;
  };



}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_PEER_CONTEXT_HPP
