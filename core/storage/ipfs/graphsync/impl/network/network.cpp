/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network.hpp"

#include <cassert>

#include "inbound_endpoint.hpp"
#include "message_reader.hpp"
#include "outbound_endpoint.hpp"
#include "peer_context.hpp"

namespace fc::storage::ipfs::graphsync {

  Network::Network(std::shared_ptr<libp2p::Host> host,
                   std::shared_ptr<libp2p::protocol::Scheduler> scheduler)
      : host_(std::move(host)),
        scheduler_(std::move(scheduler)),
        protocol_id_(kProtocolVersion) {
    assert(host_);
    assert(scheduler_);
  }

  Network::~Network() {
    closeAllPeers();
  }

  void Network::start(std::shared_ptr<PeerToGraphsyncFeedback> feedback) {
    feedback_ = std::move(feedback);
    assert(feedback_);

    // clang-format off
    host_->setProtocolHandler(
        protocol_id_,
        [wptr = weak_from_this()] (outcome::result<StreamPtr> rstream) {
          auto self = wptr.lock();
          if (self) { self->onStreamAccepted(std::move(rstream)); }
        }
    );
    // clang-format on
    started_ = true;
  }

  void Network::stop() {
    started_ = false;
    closeAllPeers();
  }

  bool Network::canSendRequest(const PeerId &peer) {
    if (!started_) {
      return false;
    }

    auto ctx = findContext(peer, true);
    if (!ctx) {
      return false;
    }
    return ctx->getState() != PeerContext::is_closed;
  }

  void Network::makeRequest(
      const PeerId &peer,
      boost::optional<libp2p::multi::Multiaddress> address,
      RequestId request_id,
      SharedData request_body) {
    if (!canSendRequest(peer)) {
      logger()->error("inconsistency in making request to network");
      return;
    }

    auto ctx = findContext(peer, false);
    assert(ctx);

    logger()->trace("makeRequest: {} has state {}", ctx->str, ctx->getState());

    ctx->setOutboundAddress(std::move(address));
    if (ctx->needToConnect()) {
      tryConnect(ctx);
    }

    ctx->enqueueRequest(request_id, std::move(request_body));
  }

  void Network::asyncFeedback(const PeerId &peer,
                              RequestId request_id,
                              ResponseStatusCode status) {
    scheduler_
        ->schedule(
            [wptr{weak_from_this()}, p = peer, id = request_id, s = status]() {
              auto self = wptr.lock();
              if (self && self->started_) {
                if (isTerminal(s)) {
                  self->active_requests_per_peer_.erase(id);
                }
                self->feedback_->onResponse(p, id, s, {});
              }
            })
        .detach();
  }

  void Network::cancelRequest(RequestId request_id, SharedData request_body) {
    if (!started_) {
      return;
    }
    auto it = active_requests_per_peer_.find(request_id);
    if (it == active_requests_per_peer_.end()) {
      return;
    }
    auto ctx = it->second;
    active_requests_per_peer_.erase(it);
    if (request_body) {
      ctx->cancelRequest(request_id, std::move(request_body));
    }
  }

  bool Network::addBlockToResponse(const PeerId &peer,
                                   RequestId request_id,
                                   const CID &cid,
                                   const common::Buffer &data) {
    if (!started_) {
      return false;
    }

    auto ctx = findContext(peer, false);
    if (!ctx) {
      return false;
    }

    return ctx->addBlockToResponse(request_id, cid, data);
  }

  void Network::sendResponse(const PeerId &peer,
                             int request_id,
                             ResponseStatusCode status,
                             const ResponseMetadata &metadata) {
    if (!started_) {
      return;
    }

    auto ctx = findContext(peer, false);
    if (!ctx) {
      return;
    }

    ctx->sendResponse(request_id, status, metadata);
  }

  void Network::peerClosed(const PeerId &peer, ResponseStatusCode status) {
    auto it = peers_.find(peer);
    if (it != peers_.end()) {
      peers_.erase(it);
    }
  }

  PeerContextPtr Network::findContext(const PeerId &peer,
                                      bool create_if_not_found) {
    assert(started_ && feedback_);

    PeerContextPtr ctx;

    auto it = peers_.find(peer);
    if (it != peers_.end()) {
      ctx = *it;
      if (ctx->getState() == PeerContext::is_closed) {
        peers_.erase(it);
        ctx.reset();
      }
    }

    if (!ctx && create_if_not_found) {
      ctx = std::make_shared<PeerContext>(peer, *feedback_, *this, *scheduler_);
      peers_.insert(ctx);
    }

    return ctx;
  }

  void Network::tryConnect(const PeerContextPtr &ctx) {
    libp2p::peer::PeerInfo pi = ctx->getOutboundPeerInfo();

    logger()->trace(
        "connecting to {}, {}",
        ctx->str,
        pi.addresses.empty() ? "''" : pi.addresses[0].getStringAddress());

    // clang-format off
    host_->newStream(
        pi,
        protocol_id_,
        [wptr{ctx->weak_from_this()}]
        (outcome::result<StreamPtr> rstream) {
          auto ctx = wptr.lock();
          if (ctx) {
            ctx->onStreamConnected(std::move(rstream));
          }
        }
    );
    // clang-format on
  }

  void Network::onStreamAccepted(outcome::result<StreamPtr> rstream) {
    if (!started_) {
      return;
    }

    if (!rstream) {
      logger()->error("accept error, msg='{}'", rstream.error().message());
      return;
    }

    auto peer_id_res = rstream.value()->remotePeerId();
    if (!peer_id_res) {
      logger()->error("no peer id for accepted stream, msg='{}'",
                      rstream.error().message());
      return;
    }

    auto ctx = findContext(peer_id_res.value(), true);

    logger()->trace("accepted stream from peer={}", ctx->str);

    ctx->onStreamAccepted(std::move(rstream.value()));
  }

  void Network::closeAllPeers() {
    PeerSet peers = std::move(peers_);
    for (auto &ctx : peers) {
      ctx->close(RS_REJECTED_LOCALLY);
    }

    // should be empty by the moment
    active_requests_per_peer_.clear();
  }

}  // namespace fc::storage::ipfs::graphsync
