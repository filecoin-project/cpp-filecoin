/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_SERVER_HPP
#define CPP_FILECOIN_GRAPHSYNC_SERVER_HPP

#include <functional>

#include <libp2p/host/host.hpp>

#include "../message.hpp"
#include "../session.hpp"

namespace fc::storage::ipfs::graphsync {

  /**
   * Accepts inbound streams and calls back to Graphsync iff client message is
   * read successfully
   */
  class Server : public std::enable_shared_from_this<Server> {
   public:
    using OnNewSession = std::function<void(SessionPtr session, Message msg)>;

    static const libp2p::peer::Protocol& getProtocolId();

    Server(std::shared_ptr<libp2p::Host> host, OnNewSession callback);

    /// Starts accepting streams
    void start();

   private:
    void onStreamAccepted(outcome::result<StreamPtr> rstream);
    void onMessageRead(const SessionPtr& from, outcome::result<Message> msg_res);

    std::shared_ptr<libp2p::Host> host_;
    OnNewSession callback_;
    std::set<SessionPtr> sessions_;
    uint64_t current_session_id_ = 0;
    bool started_ = false;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_SERVER_HPP
