/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hello.hpp"

#include <cassert>

#include "codec/cbor/cbor_decode_stream.hpp"
#include "codec/cbor/cbor_encode_stream.hpp"
#include "common/logger.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync");
      return logger.get();
    }

    const libp2p::peer::Protocol &getProtocolId() {
      static const libp2p::peer::Protocol protocol_id("/fil/hello/1.0.0");
      return protocol_id;
    }
  }  // namespace

  void Hello::start(std::shared_ptr<libp2p::Host> host,
                    std::shared_ptr<clock::UTCClock> clock,
                    CID genesis_cid,
                    const Message &initial_state,
                    HelloFeedback hello_feedback,
                    LatencyFeedback latency_feedback) {
    host_ = std::move(host);
    clock_ = std::move(clock);
    genesis_ = std::move(genesis_cid);
    hello_feedback_ = std::move(hello_feedback);
    latency_feedback_ = std::move(latency_feedback);

    assert(host_);
    assert(clock_);
    assert(genesis_);
    assert(hello_feedback_);
    assert(latency_feedback_);

    onHeadChanged(initial_state);

    // clang-format off
    host_->setProtocolHandler(
      getProtocolId(),
      [wptr = weak_from_this()] (auto rstream) {
        auto self = wptr.lock();
        if (self) { self->onAccepted(std::make_shared<CborStream>(rstream)); }
      }
    );
    // clang-format on

    log()->info("hello protocol started");
  }

  bool Hello::started() const {
    return genesis_.has_value();
  }

  void Hello::stop() {
    if (started()) {
      genesis_.reset();

      for (const auto &req : active_requests_) {
        req.second.stream->stream()->reset();
      }

      active_requests_.clear();
      active_requests_by_sent_time_.clear();
      request_body_.reset();
      current_tipset_.clear();

      log()->info("hello protocol stopped");
    }
  }

  Hello::RequestCtx::RequestCtx(clock::Time t) : sent(t) {}

  bool Hello::TimeAndPeerId::operator<(const TimeAndPeerId &other) const {
    return t < other.t;
  }

  void Hello::sayHello(const PeerId &peer_id) {
    if (!started()) {
      return;
    }
    if (active_requests_.count(peer_id) != 0) {
      return;
    }
    auto sent = clock_->nowUTC();
    active_requests_.insert(std::make_pair(peer_id, RequestCtx(sent)));
    active_requests_by_sent_time_.insert({sent, peer_id});

    // clang-format off
    host_->newStream(
        libp2p::peer::PeerInfo{ peer_id, {}},
        getProtocolId(),
        [wptr = weak_from_this(), peer_id] (auto rstream) {
          auto self = wptr.lock();
          if (self) {
            if (rstream) {
              self->onConnected(peer_id, std::make_shared<CborStream>(rstream.value()));
            } else {
              self->onConnected(peer_id, rstream.error());
            }
          }
        }
    );
    // clang-format on

    log()->debug("saying hello to {}", peer_id.toBase58());
  }

  void Hello::onHeartbeat() {
    auto expire_time = clock_->nowUTC().unixTime() - std::chrono::seconds(10);

    while (!active_requests_by_sent_time_.empty()) {
      auto it = active_requests_by_sent_time_.begin();
      if (it->t.unixTime() > expire_time) {
        break;
      }

      auto peer_id = it->p;
      active_requests_by_sent_time_.erase(it);

      auto it2 = active_requests_.find(peer_id);
      assert(it2 != active_requests_.end());

      it2->second.stream->stream()->reset();
      active_requests_.erase(it2);

      latency_feedback_(peer_id, Error::HELLO_TIMEOUT);
    }
  }

  void Hello::onHeadChanged(Message state) {
    if (current_tipset_ == state.heaviest_tipset) {
      // request body is cached and didnt change
      return;
    }

    current_tipset_ = state.heaviest_tipset;

    state.genesis = genesis_.value();
    OUTCOME_EXCEPT(body, codec::cbor::encode(state));
    request_body_ = std::make_shared<const common::Buffer>(body);
  }

  void Hello::onConnected(const PeerId &peer_id,
                          outcome::result<Hello::StreamPtr> rstream) {
    if (!rstream || rstream.value()->stream()->isClosed()) {
      clearRequest(peer_id);
      latency_feedback_(peer_id, Error::HELLO_NO_CONNECTION);
      return;
    }

    auto it = active_requests_.find(peer_id);
    if (it == active_requests_.end()) {
      return;
    }
    auto &ctx = it->second;
    ctx.stream = rstream.value();

    // clang-format off
    ctx.stream->stream()->write(
        *request_body_, request_body_->size(),
        [wptr = weak_from_this(), buf=request_body_, peer_id] (auto res) {
          auto self = wptr.lock();
          if (self) {  self->onRequestWritten(peer_id, res); }
        }
    );
    //clang-format on
  }

  void Hello::onRequestWritten(const PeerId &peer_id,
                        outcome::result<size_t> result) {
    if (!started()) {
      return;
    }

    auto it = active_requests_.find(peer_id);
    if (it == active_requests_.end()) {
      return;
    }

    if (!result) {
      clearRequest(peer_id);
      log()->error("{}", __LINE__);
      latency_feedback_(peer_id, Error::HELLO_NO_CONNECTION);
      return;
    }

    it->second.stream->read<LatencyMessage>([peer_id, self{shared_from_this()}](auto result) {
      self->onResponseRead(peer_id, result);
    });
  }

  void Hello::onResponseRead(const PeerId &peer_id,
                             outcome::result<LatencyMessage> result) {
    if (!started()) {
      return;
    }

    auto it = active_requests_.find(peer_id);
    if (it == active_requests_.end()) {
      return;
    }

    if (!result) {
      clearRequest(peer_id);
      log()->error("cannot read hello response: {}", result.error().message());
      latency_feedback_(peer_id, result.error());
      return;
    }

    auto &ctx = it->second;

    // TODO(artem): do smth with clock results

    auto latency = clock_->nowUTC().unixTimeNano() - ctx.sent.unixTimeNano();
    clearRequest(peer_id);

    log()->debug("got hello response from {}", peer_id.toBase58());

    latency_feedback_(peer_id, latency.count());
  }

  void Hello::clearRequest(const PeerId &peer_id) {
    auto it = active_requests_.find(peer_id);
    if (it == active_requests_.end()) {
      return;
    }
    auto sent = it->second.sent;
    if (it->second.stream) {
      it->second.stream->stream()->reset();
    }
    active_requests_.erase(it);
    active_requests_by_sent_time_.erase({sent, peer_id});
  }

  void Hello::onAccepted(StreamPtr stream) {
    assert(stream->stream()->remotePeerId());
    stream->read<Message>([stream, self{shared_from_this()}](auto result) {
      self->onRequestRead(stream, result);
    });
  }

  void Hello::onRequestRead(const StreamPtr &stream,
                            outcome::result<Message> result) {
    if (!started()) {
      return;
    }

    auto peer_res = stream->stream()->remotePeerId();
    if (!peer_res) {
      log()->error("Hello request: no remote peer");
      return;
    }

    if (!result) {
      log()->error("Hello request read failed: {}", result.error().message());
      hello_feedback_(peer_res.value(), result.error());
      stream->stream()->reset();
      return;
    }

    if (result.value().genesis != genesis_.value()) {
      hello_feedback_(peer_res.value(), Error::HELLO_GENESIS_MISMATCH);
      stream->stream()->reset();
      return;
    }

    auto arrival = clock_->nowUTC().unixTimeNano().count();
    hello_feedback_(peer_res.value(), std::move(result));

    auto sent = clock_->nowUTC().unixTimeNano().count();

    stream->write(LatencyMessage{arrival, sent}, [stream](auto) {
      stream->stream()->reset();
    });
  }

}  // namespace fc::sync

OUTCOME_CPP_DEFINE_CATEGORY(fc::sync, Hello::Error, e) {
  using E = fc::sync::Hello::Error;

  switch (e) {
    case E::HELLO_NO_CONNECTION:
      return "Hello protocol: no connection";
    case E::HELLO_TIMEOUT:
      return "Hello protocol: timeout";
    case E::HELLO_MALFORMED_MESSAGE:
      return "Hello protocol: malformed message";
    case E::HELLO_GENESIS_MISMATCH:
      return "Hello protocol: genesis mismatch";
    default:
      break;
  }
  return "Hello::Error: unknown error";
}
