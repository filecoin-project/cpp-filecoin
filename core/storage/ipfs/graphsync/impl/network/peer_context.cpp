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
    if (requests_endpoint_) {
      return false;
    }

    requests_endpoint_ = std::make_unique<OutboundEndpoint>();

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

    if (streams_.empty()) {
      timer_ = scheduler_.schedule(kStreamCloseDelayMsec,
                                   [wptr{weak_from_this()}]() {
                                     auto self = wptr.lock();
                                     if (self) {
                                       self->onStreamCleanupTimer();
                                     }
                                   });
    }

    shiftExpireTime(stream_ctx);

    streams_.emplace(std::move(stream), std::move(stream_ctx));
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
      if (getState() == is_connecting) {
        closeLocalRequests(RS_CANNOT_CONNECT);
      }
    }
  }

  void PeerContext::onStreamAccepted(StreamPtr stream) {
    if (closed_) {
      stream->reset();
      return;
    }
    onNewStream(std::move(stream));
  }

  void PeerContext::enqueueRequest(RequestId request_id,
                                   SharedData request_body) {
    if (!requests_endpoint_) {
      close(RS_INTERNAL_ERROR);
    }
    auto res = requests_endpoint_->enqueue(std::move(request_body));
    if (res) {
      local_request_ids_.insert(request_id);
    } else {
      logger()->error("enqueueRequest: outbound buffers overflow for peer {}",
                      str);
      close(RS_SLOW_STREAM);
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
      return false;
    }
    return true;
  }

  void PeerContext::sendResponse(RequestId request_id,
                                 ResponseStatusCode status,
                                 const std::vector<Extension> &extensions) {
    auto it = findResponseSink(request_id);
    if (it == streams_.end()) {
      return;
    }
    auto &ctx = it->second;

    createResponseEndpoint(it->first, ctx);

    auto res =
        ctx.response_endpoint->sendResponse(request_id, status, extensions);
    if (!res) {
      logger()->error("sendResponse: {}, peer={}", res.error().message(), str);

      close(RS_SLOW_STREAM);
    }
  }

  void PeerContext::close(ResponseStatusCode status) {
    if (closed_) {
      return;
    }

    logger()->debug("close peer={} status={}", str, statusCodeToString(status));

    close_status_ = status;
    closed_ = true;
    remote_requests_streams_.clear();
    while (!streams_.empty()) {
      auto s = streams_.begin()->first;
      closeStream(s, status);
    }

    if (status != RS_REJECTED_LOCALLY) {
      timer_ = scheduler_.schedule(0, [wptr{weak_from_this()}]() {
        auto self = wptr.lock();
        if (self) {
          self->network_feedback_.peerClosed(self->peer, self->close_status_);
        }
      });
    } else {
      network_feedback_.peerClosed(peer, RS_REJECTED_LOCALLY);
    }
  }

  void PeerContext::closeStream(StreamPtr stream, ResponseStatusCode status) {
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
      closeLocalRequests(status);
    }

    stream->close(
        [stream](outcome::result<void>) { logger()->trace("stream closed"); });
  }

  void PeerContext::closeLocalRequests(ResponseStatusCode status) {
    if (!local_request_ids_.empty()) {
      std::set<RequestId> ids = std::move(local_request_ids_);
      for (auto id : ids) {
        graphsync_feedback_.onResponse(peer, id, close_status_, {});
      }
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
        peer, response.id, response.status, std::move(response.extensions));
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
      closeStream(stream, RS_CONNECTION_ERROR);
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

    shiftExpireTime(it->second);
  }

  void PeerContext::onWriterEvent(const StreamPtr &stream,
                                  outcome::result<void> result) {
    if (closed_) {
      return;
    }

    if (!result) {
      logger()->info(
          "stream write error, peer={}, msg={}", str, result.error().message());
      close(RS_CONNECTION_ERROR);
      return;
    }

    shiftExpireTime(stream);
  }

  void PeerContext::shiftExpireTime(PeerContext::StreamCtx &ctx) {
    ctx.expire_time = scheduler_.now() + kStreamCloseDelayMsec;
  }

  void PeerContext::shiftExpireTime(const StreamPtr &stream) {
    auto it = streams_.find(stream);
    if (it != streams_.end()) {
      shiftExpireTime(it->second);
    }
  }

  void PeerContext::onStreamCleanupTimer() {
    uint64_t max_expire_time = 0;

    if (streams_.empty()) {
      close(RS_TIMEOUT);
      return;
    }

    auto now = scheduler_.now();

    std::vector<StreamPtr> timed_out;

    for (auto &[stream, ctx] : streams_) {
      if (ctx.queue && ctx.queue->getState().writing_bytes > 0) {
        continue;
      }
      if (ctx.expire_time <= now) {
        timed_out.push_back(stream);
      } else if (ctx.expire_time > max_expire_time) {
        max_expire_time = ctx.expire_time;
      }
    }

    for (auto &stream : timed_out) {
      closeStream(std::move(stream), RS_TIMEOUT);
    }

    if (!streams_.empty() && max_expire_time > now) {
      timer_.reschedule(max_expire_time - now);
    } else {
      timer_.reschedule(kPeerCloseDelayMsec);
    }
  }

}  // namespace fc::storage::ipfs::graphsync
