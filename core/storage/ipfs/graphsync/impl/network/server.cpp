/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "server.hpp"

#include "graphsync_message_reader.hpp"

namespace fc::storage::ipfs::graphsync {

  const libp2p::peer::Protocol &Server::getProtocolId() {
    static const libp2p::peer::Protocol id(kProtocolVersion);
    return id;
  }

  Server::Server(std::shared_ptr<libp2p::Host> host, OnNewSession callback)
      : host_(std::move(host)), callback_(std::move(callback)) {
    assert(host_);
    assert(callback_);
  }

  void Server::start() {
    // clang-format off
    host_->setProtocolHandler(
        getProtocolId(),
        [wptr = weak_from_this()] (auto rstream) {
          auto self = wptr.lock();
          if (self) { self->onStreamAccepted(std::move(rstream)); }
        }
    );
    // clang-format on
    started_ = true;
  }

  void Server::onStreamAccepted(libp2p::outcome::result<StreamPtr> rstream) {
    if (!rstream) {
      // XXX log rstream.error().message());
      return;
    }

    auto res = rstream.value()->remotePeerId();
    if (!res) {
      // XXX log
      return;
    }

    auto session = std::make_shared<Session>(
        current_session_id_, std::move(res.value()), Session::SESSION_ACCEPTED);

    // TODO(artem): config
    session->reader =
        std::make_shared<GraphsyncMessageReader>(session, 1 << 24);

    bool ok = session->reader->read(
        rstream.value(),
        [wptr{weak_from_this()}](const SessionPtr &from,
                                 outcome::result<Message> msg_res) {
          auto self = wptr.lock();
          if (self && self->started_) {
            self->onMessageRead(from, std::move(msg_res));
          }
        });

    // just accepted stream must be readable
    assert(ok);

    session->stream = std::move(rstream.value());
    sessions_.insert(session);

    // accepted streams' ids are even, connected ones are odd
    current_session_id_ += 2;
  }

  void Server::onMessageRead(const SessionPtr &from,
                             outcome::result<Message> msg_res) {
    SessionPtr session(from);
    sessions_.erase(from);

    if (!msg_res) {
      // TODO: forward peer id to owner and/or ban it for some time period
      // TODO: log
      return;
    }

    callback_(std::move(session), std::move(msg_res.value()));
  }

}  // namespace fc::storage::ipfs::graphsync
