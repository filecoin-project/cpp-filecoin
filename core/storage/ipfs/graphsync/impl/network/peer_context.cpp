/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "peer_context.hpp"

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/security/noise/crypto/state.hpp>

#include "common/libp2p/stream_read_buffer.hpp"
#include "common/ptr.hpp"
#include "message_queue.hpp"
#include "message_reader.hpp"
#include "outbound_endpoint.hpp"

namespace fc::storage::ipfs::graphsync {

  namespace {
    std::string makeStringRepr(const PeerId &peer_id) {
      return peer_id.toBase58().substr(46);
    }
  }  // namespace

  PeerContext::PeerContext(PeerId peer_id,
                           PeerToGraphsyncFeedback &graphsync_feedback,
                           PeerToNetworkFeedback &network_feedback,
                           Host &host,
                           Scheduler &scheduler)
      : peer_(std::move(peer_id)),
        str_(makeStringRepr(peer_)),
        graphsync_feedback_(graphsync_feedback),
        network_feedback_(network_feedback),
        host_(host),
        scheduler_(scheduler) {}

  // Need to define it here due to unique_ptrs to incomplete types in the header
  PeerContext::~PeerContext() {
    logger()->trace("~PeerContext, {}", str_);
    // must be closed
    if (!closed_) {
      close(RS_INTERNAL_ERROR);
    }
  }

  void PeerContext::setOutboundAddress(
      std::vector<libp2p::multi::Multiaddress> connect_to) {
    connect_to_ = std::move(connect_to);
  }

  void PeerContext::connectIfNeeded() {
    if (getState() != can_connect) {
      return;
    }

    if (!outbound_endpoint_) {
      outbound_endpoint_ = std::make_unique<OutboundEndpoint>();

      logger()->debug("connecting to {}", str_);

      // clang-format off
      host_.newStream(
          {peer_, connect_to_},
          std::string(kProtocolVersion),
          [wptr{weak_from_this()}]
              (outcome::result<StreamPtr> rstream) {
            auto ctx = wptr.lock();
            if (ctx) {
              ctx->onStreamConnected(std::move(rstream));
            }
          }
      );
      // clang-format on
    }
  }

  void PeerContext::onStreamConnected(outcome::result<StreamPtr> rstream) {
    if (closed_) {
      if (rstream) rstream.value()->reset();
      return;
    }
    if (rstream) {
      logger()->debug("connected to peer={}", str_);
      onNewStream(std::move(rstream.value()), true);
    } else {
      logger()->info(
          "cannot connect, peer={}, msg='{}'", str_, rstream.error().message());
      if (getState() == is_connecting) {
        closeLocalRequests(RS_CANNOT_CONNECT);
      }
    }
  }

  void PeerContext::onNewStream(StreamPtr stream, bool is_outbound) {
    if (stream) {
      stream = std::make_shared<libp2p::connection::StreamReadBuffer>(
          stream, libp2p::security::noise::kMaxMsgLen);
    }

    assert(stream);
    assert(streams_.count(stream) == 0);

    if (!stream || (streams_.count(stream) != 0)) {
      logger()->error("onNewStream: inconsistency, peer={}", str_);
      return;
    }

    StreamCtx stream_ctx;
    stream_ctx.reader = std::make_unique<MessageReader>(*this);

    if (is_outbound) {
      assert(getState() == is_connecting);
      assert(outbound_endpoint_);

      outbound_endpoint_->onConnected(std::make_shared<MessageQueue>(
          stream, [this](const StreamPtr &stream, outcome::result<void> res) {
            onWriterEvent(stream, res);
          }));

      assert(getState() == is_connected);
    }

    if (streams_.empty()) {
      scheduleCleanup(kStreamCloseDelayMsec);
    }

    auto [it, _] = streams_.emplace(stream, std::move(stream_ctx));
    shiftExpireTime(stream);
    it->second.reader->read(std::move(stream));
  }

  PeerContext::State PeerContext::getState() const {
    if (closed_) {
      return is_closed;
    }
    if (outbound_endpoint_) {
      if (outbound_endpoint_->isConnecting()) {
        return is_connecting;
      }
      return is_connected;
    }
    return can_connect;
  }

  const std::string &PeerContext::asString() const {
    return str_;
  }

  void PeerContext::onStreamAccepted(StreamPtr stream) {
    if (closed_) {
      logger()->debug(
          "inbound stream from peer {}, but ctx is closed, ignoring", str_);
      stream->reset();
      return;
    }
    logger()->debug("inbound stream from peer {}", str_);
    onNewStream(std::move(stream), false);
  }

  void PeerContext::enqueueRequest(RequestId request_id,
                                   SharedData request_body) {
    connectIfNeeded();
    logger()->debug(
        "enqueueing request to peer {}, size=", str_, request_body->size());
    auto res = outbound_endpoint_->enqueue(std::move(request_body));
    if (res) {
      local_request_ids_.insert(request_id);
    } else {
      logger()->info("enqueueRequest: outbound buffers overflow for peer {}",
                     str_);
      close(RS_SLOW_STREAM);
    }
  }

  void PeerContext::cancelRequest(RequestId request_id,
                                  SharedData request_body) {
    local_request_ids_.erase(request_id);
    if (outbound_endpoint_) {
      if (outbound_endpoint_->enqueue(std::move(request_body))) {
        return;
      }
      logger()->info("cancelRequest: outbound buffers overflow for peer {}",
                     str_);
    }
  }

  void PeerContext::sendResponse(const FullRequestId &id,
                                 const Response &response) {
    connectIfNeeded();
    auto res = outbound_endpoint_->sendResponse(id, response);
    if (!res) {
      logger()->error("sendResponse: {}, peer={}", res.error().message(), str_);
      close(RS_SLOW_STREAM);
    }
  }

  void PeerContext::postBlocks(RequestId request_id,
                               const Responder &responder) {
    connectIfNeeded();
    responders_.emplace(request_id, responder);
    if (outbound_endpoint_->empty()) {
      checkResponders();
    }
  }

  void PeerContext::close(ResponseStatusCode status) {
    if (closed_) {
      return;
    }

    logger()->debug(
        "close peer={} status={}", str_, statusCodeToString(status));

    for (auto &p : responders_) {
      p.second(false);
    }
    responders_.clear();

    close_status_ = status;
    closed_ = true;
    while (!streams_.empty()) {
      auto s = streams_.begin()->first;
      closeStream(s, status);
    }

    if (status != RS_REJECTED_LOCALLY) {
      scheduler_.schedule(
          weakCb(*this, [](std::shared_ptr<PeerContext> &&self) {
            self->network_feedback_.peerClosed(self->peer_,
                                               self->close_status_);
          }));
    } else {
      network_feedback_.peerClosed(peer_, RS_REJECTED_LOCALLY);
    }
  }

  // here stream should copy, because streams_.erase will reset original one
  // NOLINTNEXTLINE
  void PeerContext::closeStream(StreamPtr stream, ResponseStatusCode status) {
    auto it = streams_.find(stream);
    if (it == streams_.end()) {
      logger()->error("closeStream: stream not found, peer={}", str_);
      return;
    }

    logger()->debug(
        "closeStream: peer={}, {}", str_, statusCodeToString(status));

    streams_.erase(it);

    if (outbound_endpoint_ && outbound_endpoint_->getStream() == stream) {
      closeLocalRequests(status);
      outbound_endpoint_.reset();
    }

    stream->reset();
    logger()->trace("stream closed");
  }

  void PeerContext::closeLocalRequests(ResponseStatusCode status) {
    if (!local_request_ids_.empty()) {
      std::set<RequestId> ids = std::move(local_request_ids_);
      for (auto id : ids) {
        graphsync_feedback_.onResponse(peer_, id, close_status_, {});
      }
    }
  }

  void PeerContext::onResponse(Message::Response &response) {
    auto it = local_request_ids_.find(response.id);
    if (it == local_request_ids_.end()) {
      logger()->info(
          "ignoring response for unexpected request id={} from peer {}",
          response.id,
          str_);
    }

    logger()->debug(
        "response from peer={}, {}", str_, statusCodeToString(response.status));

    if (isTerminal(response.status)) {
      local_request_ids_.erase(it);
    }

    graphsync_feedback_.onResponse(
        peer_, response.id, response.status, std::move(response.extensions));
  }

  void PeerContext::onRequest(const StreamPtr &stream,
                              Message::Request &request) {
    auto it = streams_.find(stream);
    if (it == streams_.end()) {
      logger()->error("onRequest: stream not found, peer={}", str_);
      return;
    }

    if (request.cancel) {
      remote_request_ids_.erase(request.id);
      logger()->debug(
          "onRequest: peer {} cancelled request {}", str_, request.id);
    } else {
      if (remote_request_ids_.count(request.id) != 0) {
        sendResponse(FullRequestId{peer_, request.id},
                     Response{RS_REJECTED, {}, {}});
      } else {
        remote_request_ids_.emplace(request.id);
        logger()->debug(
            "onRequest: peer {} created request {}", str_, request.id);
        graphsync_feedback_.onRemoteRequest(peer_, std::move(request));
      }
    }
  }

  void PeerContext::onReaderEvent(const StreamPtr &stream,
                                  outcome::result<Message> msg_res) {
    if (closed_) {
      return;
    }

    if (!msg_res) {
      logger()->info("stream read error, peer={}, msg={}",
                     str_,
                     msg_res.error().message());
      closeStream(stream, RS_CONNECTION_ERROR);
      return;
    }

    Message &msg = msg_res.value();

    logger()->trace(
        "message from peer={}, {} blocks, {} requests, {} responses",
        str_,
        msg.data.size(),
        msg.requests.size(),
        msg.responses.size());

    if (msg.complete_request_list) {
      // this cancels previous requests
      remote_request_ids_.clear();
    }

    for (auto &item : msg.requests) {
      onRequest(stream, item);
    }

    for (auto &item : msg.data) {
      graphsync_feedback_.onDataBlock(
          peer_, {std::move(item.first), std::move(item.second)});
    }

    for (auto &item : msg.responses) {
      onResponse(item);
    }

    shiftExpireTime(stream);
  }

  void PeerContext::onWriterEvent(const StreamPtr &stream,
                                  outcome::result<void> result) {
    if (closed_) {
      return;
    }

    if (!result) {
      logger()->info("stream write error, peer={}, msg={}",
                     str_,
                     result.error().message());
      close(RS_CONNECTION_ERROR);
      return;
    }

    checkResponders();

    shiftExpireTime(stream);
  }

  void PeerContext::shiftExpireTime(const StreamPtr &stream) {
    auto it = streams_.find(stream);
    if (it != streams_.end()) {
      it->second.expire_time = scheduler_.now() + kStreamCloseDelayMsec;
    }
  }

  void PeerContext::onStreamCleanupTimer() {
    if (closed_) {
      return;
    }

    std::chrono::milliseconds max_expire_time =
        std::chrono::milliseconds::zero();

    if (streams_.empty()) {
      close(RS_TIMEOUT);
      return;
    }

    auto now = scheduler_.now();

    std::vector<StreamPtr> timed_out;

    for (auto &[stream, ctx] : streams_) {
      if (outbound_endpoint_ && outbound_endpoint_->getStream() == stream) {
        continue;
      }
      if (ctx.expire_time <= now) {
        timed_out.push_back(stream);
      } else if (ctx.expire_time > max_expire_time) {
        max_expire_time = ctx.expire_time;
      }
    }

    for (auto &stream : timed_out) {
      closeStream(stream, RS_TIMEOUT);
    }

    if (!streams_.empty() && max_expire_time > now) {
      scheduleCleanup(max_expire_time - now);
    } else {
      scheduleCleanup(kPeerCloseDelayMsec);
    }
  }

  void PeerContext::checkResponders() {
    auto it{responders_.begin()};
    while (it != responders_.end()) {
      auto &[id, cb]{*it};
      auto res{cb(true)};
      if (res) {
        sendResponse({peer_, id}, *res);
      }
      if (!res || isTerminal(res->status)) {
        it = responders_.erase(it);
      }
      if (res) {
        break;
      }
    }
  }

  void PeerContext::scheduleCleanup(std::chrono::milliseconds delay) {
    scheduler_.schedule(weakCb(*this,
                               [](std::shared_ptr<PeerContext> &&self) {
                                 self->onStreamCleanupTimer();
                               }),
                        delay);
  }
}  // namespace fc::storage::ipfs::graphsync
