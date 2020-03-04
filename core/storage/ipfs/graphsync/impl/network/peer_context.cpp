/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "peer_context.hpp"

#include <libp2p/connection/stream.hpp>

#include "inbound_endpoint.hpp"
#include "message_queue.hpp"
#include "message_reader.hpp"
#include "outbound_endpoint.hpp"

namespace fc::storage::ipfs::graphsync {

  namespace {

    std::string makeStringRepr(const PeerId &peer_id) {
      return peer_id.toBase58().substr(46);
    }

    /// Needed for sets and maps
    inline bool less(const PeerId &a, const PeerId &b) {
      // N.B. toVector returns const std::vector&, i.e. it is fast
      return a.toVector() < b.toVector();
    }

  }  // namespace

  PeerContext::PeerContext(PeerId peer_id,
                           PeerToGraphsyncFeedback &graphsync_feedback,
                           PeerToNetworkFeedback &network_feedback,
                           libp2p::protocol::Scheduler &scheduler)
      : peer(std::move(peer_id)),
        str(makeStringRepr(peer)),
        graphsync_feedback_(graphsync_feedback),
        network_feedback_(network_feedback),
        scheduler_(scheduler) {}

  // Need to define it here due to unique_ptrs to incomplete types in the header
  PeerContext::~PeerContext() {
    logger()->trace("~PeerContext, {}", str);
    // must be closed
    if (!closed_) {
      close(RS_INTERNAL_ERROR);
    }
  }

  bool operator<(const PeerContextPtr &ctx, const PeerId &peer) {
    if (!ctx) return false;
    return less(ctx->peer, peer);
  }

  bool operator<(const PeerId &peer, const PeerContextPtr &ctx) {
    if (!ctx) return false;
    return less(peer, ctx->peer);
  }

  bool operator<(const PeerContextPtr &a, const PeerContextPtr &b) {
    if (!a || !b) return false;
    return less(a->peer, b->peer);
  }

  void PeerContext::setOutboundAddress(
      boost::optional<libp2p::multi::Multiaddress> connect_to) {
    if (connect_to) {
      connect_to_ = std::move(connect_to);
    }
  }

  bool PeerContext::needToConnect() {
    if (!requests_endpoint_) {
      requests_endpoint_ = std::make_unique<OutboundEndpoint>();
    }

    if (streams_.empty()) {
      return true;
    }

    auto it = streams_.begin();
    createMessageQueue(it->first, it->second);
    requests_endpoint_->onConnected(it->second.queue);

    return false;
  }

  PeerContext::State PeerContext::getState() const {
    State state = can_connect;
    if (closed_) {
      state = is_closed;
    } else if (!streams_.empty()) {
      state = is_connected;
    } else if (requests_endpoint_ && requests_endpoint_->isConnecting()) {
      state = is_connecting;
    }

    return state;
  }

  libp2p::peer::PeerInfo PeerContext::getOutboundPeerInfo() const {
    libp2p::peer::PeerInfo pi{peer, {}};
    if (connect_to_) {
      pi.addresses.push_back(connect_to_.value());
    }
    return pi;
  }

  void PeerContext::onNewStream(StreamPtr stream) {
    assert(stream);
    assert(streams_.count(stream) == 0);

//    static size_t TOTAL_STREAMS = 0;
//    ++TOTAL_STREAMS;
//    logger()->trace("TOTAL_STREAMS {}", TOTAL_STREAMS);

    if (!stream || streams_.count(stream)) {
      logger()->error("onNewStream: inconsistency, peer={}", str);
      return;
    }

    StreamCtx stream_ctx;
    stream_ctx.reader = std::make_unique<MessageReader>(stream, *this);

    if (getState() == is_connecting) {
      assert(requests_endpoint_);

      createMessageQueue(stream, stream_ctx);
      requests_endpoint_->onConnected(stream_ctx.queue);
    }

    streams_.emplace(std::move(stream), std::move(stream_ctx));

    schedulePeerClose();
  }

  void PeerContext::onStreamConnected(outcome::result<StreamPtr> rstream) {
    if (closed_) {
      return;
    }
    if (rstream) {
      logger()->debug("connected to peer={}", str);
      onNewStream(std::move(rstream.value()));
    } else {
      logger()->error(
          "cannot connect, peer={}, msg='{}'", str, rstream.error().message());
    }
  }

  void PeerContext::onStreamAccepted(StreamPtr stream) {
    if (closed_) {
      return;
    }
    onNewStream(std::move(stream));
  }

  void PeerContext::enqueueRequest(RequestId request_id,
                                   SharedData request_body) {
    if (!requests_endpoint_) {
      close(RS_INTERNAL_ERROR);
      schedulePeerClose();
    }
    auto res = requests_endpoint_->enqueue(std::move(request_body));
    if (res) {
      local_request_ids_.insert(request_id);
    } else {
      logger()->error("enqueueRequest: outbound buffers overflow for peer {}",
                      str);
      close(RS_SLOW_STREAM);
      schedulePeerClose();
    }
  }

  void PeerContext::cancelRequest(RequestId request_id,
                                  SharedData request_body) {
    local_request_ids_.erase(request_id);
    if (requests_endpoint_) {
      if (requests_endpoint_->enqueue(std::move(request_body))) {
        return;
      }
      logger()->error("cancelRequest: outbound buffers overflow for peer {}",
                      str);
      checkIfClosable(requests_endpoint_->getStream());
    }
  }

  PeerContext::Streams::iterator PeerContext::findResponseSink(
      RequestId request_id) {
    auto r_iter = remote_requests_streams_.find(request_id);
    if (r_iter == remote_requests_streams_.end()) {
      logger()->debug(
          "findResponseSink: remote request {} is no longer actual, peer={}",
          request_id,
          str);
      return streams_.end();
    }
    auto s_iter = streams_.find(r_iter->second);
    if (s_iter == streams_.end()) {
      logger()->error("findResponseSink: cannot find stream, peer={}", str);
    }
    return s_iter;
  }

  bool PeerContext::addBlockToResponse(RequestId request_id,
                                       const CID &cid,
                                       const common::Buffer &data) {
    auto it = findResponseSink(request_id);
    if (it == streams_.end()) {
      return false;
    }
    auto &ctx = it->second;

    createResponseEndpoint(it->first, ctx);

    auto res = ctx.response_endpoint->addBlockToResponse(request_id, cid, data);
    if (!res) {
      logger()->error(
          "addBlockToResponse: {}, peer={}", res.error().message(), str);

      close(RS_SLOW_STREAM);
      schedulePeerClose();

      return false;
    }
    return true;
  }

  void PeerContext::sendResponse(RequestId request_id,
                                 ResponseStatusCode status,
                                 const ResponseMetadata &metadata) {
    auto it = findResponseSink(request_id);
    if (it == streams_.end()) {
      return;
    }
    auto &ctx = it->second;

    createResponseEndpoint(it->first, ctx);

    auto res =
        ctx.response_endpoint->sendResponse(request_id, status, metadata);
    if (!res) {
      logger()->error("sendResponse: {}, peer={}", res.error().message(), str);

      close(RS_SLOW_STREAM);
      schedulePeerClose();
    }
  }

  void PeerContext::close(ResponseStatusCode status) {
    if (closed_) {
      return;
    }

    logger()->debug("close peer={} status={}", str, status);

    close_status_ = status;
    closed_ = true;
    remote_requests_streams_.clear();
    while (!streams_.empty()) {
      auto s = streams_.begin()->first;
      closeStream(s);
    }
  }

  void PeerContext::closeStream(StreamPtr stream) {
    auto it = streams_.find(stream);
    if (it == streams_.end()) {
      logger()->error("closeStream: stream not found, peer={}", str);
      return;
    }

    logger()->trace("closeStream: peer={}", str);

    for (auto id : it->second.remote_request_ids) {
      remote_requests_streams_.erase(id);
    }

    streams_.erase(it);

    if (requests_endpoint_ && requests_endpoint_->getStream() == stream) {
      closeLocalRequests();
    }

    stream->close([stream](outcome::result<void>) {});
  }

  void PeerContext::closeLocalRequests() {
    if (!local_request_ids_.empty()) {
      for (auto id : local_request_ids_) {
        graphsync_feedback_.onResponse(peer, id, close_status_, {});
      }
      local_request_ids_.clear();
    }
    requests_endpoint_.reset();
  }

  void PeerContext::onResponse(Message::Response &response) {
    auto it = local_request_ids_.find(response.id);
    if (it == local_request_ids_.end()) {
      logger()->info(
          "ignoring response for unexpected request id={} from peer {}",
          response.id,
          str);
    }
    if (isTerminal(response.status)) {
      local_request_ids_.erase(it);
    }
    graphsync_feedback_.onResponse(
        peer, response.id, response.status, std::move(response.metadata));
  }

  void PeerContext::onRequest(const StreamPtr &stream,
                              Message::Request &request) {
    auto it = streams_.find(stream);
    if (it == streams_.end()) {
      logger()->error("onRequest: stream not found, peer={}", str);
      return;
    }
    StreamCtx &ctx = it->second;

    if (request.cancel) {
      ctx.remote_request_ids.erase(request.id);
      remote_requests_streams_.erase(request.id);
      logger()->debug(
          "onRequest: peer {} cancelled request {}", str, request.id);
    } else {
      createResponseEndpoint(stream, ctx);
      if (remote_requests_streams_.count(request.id)) {
        sendResponse(request.id, RS_REJECTED, {});
      } else {
        remote_requests_streams_.emplace(request.id, stream);
        ctx.remote_request_ids.insert(request.id);
        logger()->debug(
            "onRequest: peer {} created request {}", str, request.id);
        graphsync_feedback_.onRemoteRequest(peer, std::move(request));
      }
    }
  }

  void PeerContext::createMessageQueue(const StreamPtr &stream,
                                       PeerContext::StreamCtx &ctx) {
    if (!ctx.queue) {
      ctx.queue = std::make_shared<MessageQueue>(
          stream, [this](const StreamPtr &stream, outcome::result<void> res) {
            onWriterEvent(stream, res);
          });
    }
  }

  void PeerContext::createResponseEndpoint(const StreamPtr &stream,
                                           PeerContext::StreamCtx &ctx) {
    createMessageQueue(stream, ctx);
    if (!ctx.response_endpoint) {
      ctx.response_endpoint = std::make_unique<InboundEndpoint>(ctx.queue);
    }
  }

  void PeerContext::onReaderEvent(const StreamPtr &stream,
                                  outcome::result<Message> msg_res) {
    if (closed_) {
      return;
    }

    if (!msg_res) {
      logger()->info(
          "stream read error, peer={}, msg={}", str, msg_res.error().message());
      closeStream(stream);
      schedulePeerClose();
      return;
    }

    Message &msg = msg_res.value();

    logger()->trace(
        "message from peer={}, {} blocks, {} requests, {} responses",
        str,
        msg.data.size(),
        msg.requests.size(),
        msg.responses.size());

    auto it = streams_.find(stream);
    if (it == streams_.end()) {
      logger()->error("onReaderEvent: stream not found, peer={}", str);
      return;
    }

    if (msg.complete_request_list) {
      for (auto id : it->second.remote_request_ids) {
        remote_requests_streams_.erase(id);
      }
      it->second.remote_request_ids.clear();
    }

    for (auto &item : msg.requests) {
      onRequest(stream, item);
    }

    for (auto &item : msg.data) {
      graphsync_feedback_.onBlock(
          peer, std::move(item.first), std::move(item.second));
    }

    for (auto &item : msg.responses) {
      onResponse(item);
    }

    checkIfClosable(stream, it->second);
  }

  void PeerContext::onWriterEvent(const StreamPtr &stream,
                                  outcome::result<void> result) {
    if (closed_) {
      return;
    }

    if (!result) {
      logger()->info(
          "stream write error, peer={}, msg={}", str, result.error().message());
      closeStream(stream);
      schedulePeerClose();
      return;
    }

    checkIfClosable(stream);
  }

  void PeerContext::checkIfClosable(const StreamPtr &stream,
                                    PeerContext::StreamCtx &ctx) {
    if (!ctx.remote_request_ids.empty()) {
      return;
    }

    if (ctx.queue && ctx.queue->getState().writing_bytes > 0) {
      return;
    }

    if (requests_endpoint_ && requests_endpoint_->getStream() == stream) {
      if (!local_request_ids_.empty()) {
        return;
      }
    }

    schedulePeerClose();
  }

  void PeerContext::checkIfClosable(const StreamPtr &stream) {
    auto it = streams_.find(stream);
    if (it != streams_.end()) {
      checkIfClosable(stream, it->second);
    }
  }

  void PeerContext::schedulePeerClose() {
    if (!closed_ && close_time_ != 0) {
      // just shift time
      close_time_ = scheduler_.now() + kPeerCloseDelayMsec;
      return;
    }

    // if already closed then do cleanup in the next event loop cycle,
    // set timeout otherwise
    uint64_t delay = closed_ ? 0 : kPeerCloseDelayMsec;
    close_time_ = scheduler_.now() + delay;
    close_timer_ = scheduler_.schedule(delay, [wptr{weak_from_this()}]() {
      auto self = wptr.lock();
      if (self) {
        self->onPeerCloseTimer();
      }
    });
  }

  void PeerContext::onPeerCloseTimer() {
    if (closed_) {
      network_feedback_.peerClosed(peer, close_status_);
      return;
    }

    auto now = scheduler_.now();
    if (now < close_time_) {
      close_timer_.reschedule(close_time_ - now);
      return;
    }

    if (streams_.empty()) {
      close(RS_TIMEOUT);
      network_feedback_.peerClosed(peer, close_status_);
    }
  }

}  // namespace fc::storage::ipfs::graphsync
