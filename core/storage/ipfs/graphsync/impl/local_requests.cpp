/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "local_requests.hpp"

#include <cassert>

#include "marshalling/request_builder.hpp"
#include "network/connector.hpp"
#include "network/out_msg_queue.hpp"
#include "session.hpp"

namespace fc::storage::ipfs::graphsync {

  Subscription LocalRequests::makeRequest(
      const libp2p::peer::PeerId& peer,
      boost::optional<libp2p::multi::Multiaddress> address,
      gsl::span<const uint8_t> root_cid,
      gsl::span<const uint8_t> selector,
      bool need_metadata,
      const std::vector<CID>& dont_send_cids,
      Graphsync::RequestProgressCallback callback) {
    SessionPtr session = findOrCreateSession(peer);
    assert(session->request_builder);

    int request_id = ++current_request_id_;
    session->request_builder->addRequest(request_id,
                                         root_cid,
                                         selector,
                                         need_metadata,
                                         dont_send_cids);

    session->active_requests_[request_id] = std::move(callback);

    if (!session->stream) {
      session->connect_to = std::move(address);
      connector_->tryConnect(std::move(session));
    } else {
      flushOutMessages(session);
    }

    uint64_t ticket = ++current_ticket_;
    sessions_by_ticket_[ticket] = std::move(session);
    request_by_ticket_[ticket] = request_id;

    return Subscription(ticket, weak_from_this());
  }

  SessionPtr LocalRequests::findOrCreateSession(const PeerId &peer) {
    auto it = sessions_by_peer_.find(peer);
    if (it != sessions_by_peer_.end()) {
      return it->second;
    }

    SessionPtr session = std::make_shared<Session>(
        current_session_id_, peer, Session::SESSION_DISCONNECTED);

    session->request_builder = std::make_shared<RequestBuilder>();

    // for outgoing sessions, ids are always odd
    current_session_id_ += 2;

    return session;
  }

  void LocalRequests::flushOutMessages(const SessionPtr &session) {
    assert(session->stream);
    assert(session->request_builder);

    if (session->request_builder->empty()) {
      // nothing to flush
      return;
    }

    if (!session->out_queue) {
      session->out_queue = std::make_shared<OutMessageQueue>(
          session,
          [wptr{weak_from_this()}, this](const SessionPtr &from,
                                         outcome::result<void> error) {
            auto self = wptr.lock();
            if (self) {
              onConnectionError(from, error);
            }
          },
          session->stream,
          // TODO(artem): config max_pending_bytes
          64 * 1024 * 1024);
    }

    auto serialize_res = session->request_builder->serialize();
    if (!serialize_res) {
      return asyncPostError(session, serialize_res.error());
    }

    auto res = session->out_queue->write(serialize_res.value());
    if (!res) {
      return asyncPostError(session, res);
    }
  }

  void LocalRequests::unsubscribe(uint64_t ticket) {
    auto it1 = sessions_by_ticket_.find(ticket);
    if (it1 == sessions_by_ticket_.end()) {
      // reentrancy...
      return;
    }
    SessionPtr session = it1->second;
    sessions_by_ticket_.erase(it1);

    int request_id = 0;
    auto it2 = request_by_ticket_.find(ticket);
    if (it2 != request_by_ticket_.end()) {
      request_id = it2->second;
      request_by_ticket_.erase(it2);
    } else {
      // TODO: log
    }

    if (session->active_requests_.count(request_id)) {
      session->active_requests_.erase(request_id);

      if (session->stream) {
        assert(session->request_builder);

        session->request_builder->addCancelRequest(request_id);
        return flushOutMessages(session);
      }
    }

    if (session->active_requests_.empty()) {
      closeSession(std::move(session));
    }
  }

  void LocalRequests::closeSession(SessionPtr session) {
    session->state = Session::SESSION_DISCONNECTED;
    if (session->stream) {
      session->stream->reset();
      if (session->out_queue) {
        session->out_queue->close();
      }
    }
    sessions_by_peer_.erase(session->peer);

    // TODO(artem): lifetimes
  }

}  // namespace fc::storage::ipfs::graphsync
