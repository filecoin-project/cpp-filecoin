/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_SESSION_HPP
#define CPP_FILECOIN_GRAPHSYNC_SESSION_HPP

#include <libp2p/multi/multiaddress.hpp>

#include "types.hpp"

namespace fc::storage::ipfs::graphsync {

  class GraphsyncMessageReader;
  class OutMessageQueue;

  struct Session {
    enum State {
      SESSION_ACCEPTED,
      SESSION_RECEIVED_REQUEST,
      SESSION_SENDING_RESPONSE,
      SESSION_CONNECTING,
      SESSION_SENDING_REQUEST,
      SESSION_RECEIVING_RESPONSE,
    };

    ~Session() = default;
    Session(Session &&) = delete;
    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;
    Session &operator=(Session &&) = delete;

    /// The only ctor, since Session is to be used in shared_ptr only
    explicit Session(uint64_t session_id, PeerId peer_id, State initial_state);

    /// Session Id: ids of outbound sessions are odd, even for inbound ones
    const uint64_t id;

    /// Remote peer
    const PeerId peer;

    /// String representation for loggers and debug purposes
    const std::string str;

    State state;

    boost::optional<libp2p::multi::Multiaddress> connect_to;

    StreamPtr stream;

    std::shared_ptr<GraphsyncMessageReader> reader;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_SESSION_HPP
