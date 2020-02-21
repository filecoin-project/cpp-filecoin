/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network.hpp"

#include <cassert>

#include "peer_context.hpp"

namespace fc::storage::ipfs::graphsync {

  constexpr uint64_t kConnectedEndpointTag = 0;

  Network::Network(std::shared_ptr<libp2p::Host> host,
                   std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
                   std::shared_ptr<NetworkEvents> feedback)
      : host_(std::move(host)),
        scheduler_(std::move(scheduler)),
        feedback_(std::move(feedback)) {}

  Network::~Network() {}

  void Network::start() {
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

  void Network::makeRequest(
      const PeerId &peer,
      boost::optional<libp2p::multi::Multiaddress> address,
      int request_id,
      SharedData request_body) {
    auto ctx = findOrCreateContext(peer);
    if (ctx == dont_connect_to_ || !started_) {
      // TODO (artem) schedule error
      return;
    }

    if (!ctx->connected_endpoint) {
      ctx->connected_endpoint =
          std::make_shared<Endpoint>(ctx, kConnectedEndpointTag, *this);
    }
    ctx->local_request_ids.insert(request_id);
    ctx->connected_endpoint->enqueue(std::move(request_body));
    if (!ctx->is_connecting) {
      ctx->connect_to = std::move(address);
      tryConnect(std::move(ctx));
    }
  }

  namespace {
    template <typename PeerSet, typename Key>
    PeerContextPtr findPeerCtx(const PeerSet &peers, const Key &peer) {
      auto it = peers.find(peer);
      return (it == peers.end()) ? nullptr : *it;
    }

    template <typename PeerSet, typename Key>
    boost::optional<PeerContext::ResponseCtx &> findResponseEndpoint(
        const PeerSet &peers, const Key &peer, uint64_t tag) {
      auto ctx = findPeerCtx(peers, peer);
      if (!ctx) {
        return boost::none;
      }
      auto it = ctx->accepted_endpoints.find(tag);
      if (it == ctx->accepted_endpoints.end()) {
        return boost::none;
      }
      return it->second;
    }

  }  // namespace

  void Network::addBlockToResponse(const PeerId &peer,
                                   uint64_t tag,
                                   const CID &cid,
                                   gsl::span<const uint8_t> &data) {
    if (!started_) {
      return;
    }

    auto ctx = findResponseEndpoint(peers_, peer, tag);
    if (!ctx) {
      // TODO(artem): log, session no more actual
      return;
    }
    ctx->response_builder.addDataBlock(cid, data);
  }

  void Network::sendResponse(const PeerId &peer,
                             uint64_t tag,
                             int request_id,
                             ResponseStatusCode status,
                             const ResponseMetadata &metadata) {
    if (!started_) {
      return;
    }

    auto ctx = findResponseEndpoint(peers_, peer, tag);
    if (!ctx) {
      // TODO(artem): log, session no more actual
      return;
    }
    ctx->response_builder.addResponse(request_id, status, metadata);
    auto bytes = ctx->response_builder.serialize();
    ctx->response_builder.clear();
    if (bytes) {
      ctx->endpoint->enqueue(bytes.value());
    } else {
      // TODO log
      // TODO schedule answer
    }
  }

  void Network::onReadEvent(const PeerContextPtr &from,
                            uint64_t endpoint_tag,
                            outcome::result<Message> msg_res) {
    if (!started_) {
      return;
    }

    if (!msg_res) {
      return onEndpointError(from, endpoint_tag, msg_res.error());
    }

    Message &msg = msg_res.value();

    for (auto &item : msg.data) {
      feedback_->onBlock(
          from->peer, std::move(item.first), std::move(item.second));
    }

    auto ctx = findPeerCtx(peers_, from);
    if (!ctx) {
      // no more such a context
      return;
    }

    dont_connect_to_ = from;

    for (auto &item : msg.responses) {
      auto it = ctx->local_request_ids.find(item.id);
      if (it == ctx->local_request_ids.end()) {
        // TODO log
        continue;
      }
      if (isTerminal(item.status)) {
        ctx->local_request_ids.erase(it);
      }
      feedback_->onLocalRequestProgress(
          from->peer, item.id, item.status, std::move(item.metadata));
    }

    if (!msg.requests.empty()) {
      if (endpoint_tag == kConnectedEndpointTag) {
        // make new session if requests came from outbound endpoint

        auto stream = ctx->connected_endpoint->getStream();
        endpoint_tag = makeInboundEndpoint(ctx, std::move(stream));
      }

      for (auto &item : msg.requests) {
        feedback_->onRemoteRequest(ctx->peer, endpoint_tag, std::move(item));
      }
    }

    dont_connect_to_.reset();

    // TODO close if closable
  }

  void Network::onWriteEvent(const PeerContextPtr &from,
                             uint64_t endpoint_tag,
                             outcome::result<void> result) {
    if (!started_) {
      return;
    }

    if (!result) {
      return onEndpointError(from, endpoint_tag, result);
    }

    // TODO timeouts
  }

  void Network::onEndpointError(const PeerContextPtr &from,
                                uint64_t endpoint_tag,
                                outcome::result<void> res) {
    if (!started_) {
      return;
    }

    assert(res.has_error());

    auto ctx = findPeerCtx(peers_, from);
    if (!ctx) {
      // no more such a context
      return;
    }

    if (endpoint_tag != kConnectedEndpointTag) {
      auto it = ctx->accepted_endpoints.find(endpoint_tag);
      if (it != ctx->accepted_endpoints.end()) {
        auto stream = it->second.endpoint->getStream();
        decStreamRef(stream);
        ctx->accepted_endpoints.erase(endpoint_tag);
      }
      return;
    }

    if (ctx->connected_endpoint) {
      ResponseStatusCode status = errorToStatusCode(res.error());
      closeLocalRequestsForPeer(ctx, status);
    }

    // TODO close if closable
  }

  PeerContextPtr Network::findOrCreateContext(const PeerId &peer) {
    auto it = peers_.find(peer);
    if (it != peers_.end()) {
      return *it;
    }
    PeerContextPtr ctx = std::make_shared<PeerContext>(peer);
    peers_.insert(ctx);
    return ctx;
  }

  void Network::tryConnect(PeerContextPtr ctx) {
    if (ctx->connected_endpoint->is_connected()) {
      return;
    }

    ctx->is_connecting = true;

    libp2p::peer::PeerInfo pi{ctx->peer, {}};
    if (ctx->connect_to) {
      pi.addresses.push_back(ctx->connect_to.value());
    }

    // clang-format off
    host_->newStream(
        pi,
        protocol_id_,
        [wptr{weak_from_this()}, ctx=std::move(ctx)]
        (outcome::result<StreamPtr> rstream) {
          auto self = wptr.lock();
          if (self) {
            self->onStreamConnected(ctx, std::move(rstream));
          }
        }
    );
    // clang-format on
  }

  void Network::onStreamConnected(const PeerContextPtr &ctx,
                                  outcome::result<StreamPtr> rstream) {
    if (!ctx->is_connecting) {
      // connect has been cancelled from outside
      return;
    }

    ctx->is_connecting = false;

    if (rstream) {
      incStreamRef(rstream.value());
      ctx->connected_endpoint->setStream(std::move(rstream.value()));
    } else {
      closeLocalRequestsForPeer(ctx);
    }

    // TODO close if closable
  }

  void Network::onStreamAccepted(outcome::result<StreamPtr> rstream) {
    if (!started_) {
      return;
    }

    if (!rstream) {
      // XXX log rstream.error().message());
      return;
    }

    auto peer_id_res = rstream.value()->remotePeerId();
    if (!peer_id_res) {
      // XXX log
      return;
    }

    auto ctx = findOrCreateContext(peer_id_res.value());
    makeInboundEndpoint(ctx, std::move(rstream.value()));
  }

  uint64_t Network::makeInboundEndpoint(const PeerContextPtr &ctx,
                                        StreamPtr stream) {
    incStreamRef(stream);
    uint64_t tag = ctx->current_endpoint_tag++;
    auto endpoint = std::make_shared<Endpoint>(ctx, tag, *this);
    endpoint->setStream(std::move(stream));
    ctx->accepted_endpoints[tag].endpoint = std::move(endpoint);
    return tag;
  }

  void Network::incStreamRef(const StreamPtr &stream) {
    if (stream) {
      ++stream_refs_[stream];
    }
  }

  void Network::decStreamRef(const StreamPtr &stream) {
    auto it = stream_refs_.find(stream);
    if (it == stream_refs_.end()) {
      // XXX log
      return;
    }

    assert(it->second > 0);

    if (--it->second == 0) {
      stream_refs_.erase(it);
      stream->close([stream{stream}](outcome::result<void>) {});
    }
  }

  void Network::closeLocalRequestsForPeer(const PeerContextPtr &peer,
                                          ResponseStatusCode status) {
    peer->connected_endpoint->clearOutQueue();

    dont_connect_to_ = peer;
    std::set<int> request_ids;
    std::swap(request_ids, peer->local_request_ids);
    for (int id : request_ids) {
      feedback_->onLocalRequestProgress(peer->peer, id, RS_CANNOT_CONNECT, {});
    }

    dont_connect_to_.reset();
  }

}  // namespace fc::storage::ipfs::graphsync
