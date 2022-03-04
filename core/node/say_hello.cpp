/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/say_hello.hpp"

#include <cassert>

#include "codec/cbor/cbor_decode_stream.hpp"
#include "codec/cbor/cbor_encode_stream.hpp"
#include "common/libp2p/timer_loop.hpp"
#include "common/logger.hpp"
#include "node/events.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("say_hello");
      return logger.get();
    }

    const libp2p::peer::Protocol &getProtocolId() {
      static const libp2p::peer::Protocol protocol_id(kHelloProtocol);
      return protocol_id;
    }

    constexpr std::chrono::milliseconds kHeartbeatInterval{10000};
  }  // namespace

  SayHello::SayHello(std::shared_ptr<libp2p::Host> host,
                     std::shared_ptr<Scheduler> scheduler,
                     std::shared_ptr<clock::UTCClock> clock)
      : host_(std::move(host)),
        scheduler_(std::move(scheduler)),
        clock_(std::move(clock)) {
    assert(host_);
    assert(scheduler_);
    assert(clock_);
  }

  void SayHello::start(CID genesis, std::shared_ptr<events::Events> events) {
    genesis_ = std::move(genesis);
    events_ = std::move(events);

    assert(events_);

    peer_connected_event_ = events_->subscribePeerConnected(
        [wptr = weak_from_this()](const events::PeerConnected &e) {
          auto self = wptr.lock();
          if (self) {
            if (e.protocols.count(getProtocolId())) {
              self->sayHello(e.peer_id);
            } else {
              log()->debug("peer {} doesn't handle {}, ignoring",
                           e.peer_id.toBase58(),
                           kHelloProtocol);
            }
          }
        });

    current_head_event_ = events_->subscribeCurrentHead(
        [wptr = weak_from_this()](const events::CurrentHead &e) {
          auto self = wptr.lock();
          if (self) {
            HelloMessage m;
            m.heaviest_tipset = e.tipset->key.cids();
            m.heaviest_tipset_height = e.tipset->height();
            m.heaviest_tipset_weight = e.weight;
            m.genesis = self->genesis_.value();
            OUTCOME_EXCEPT(body, codec::cbor::encode(m));
            self->request_body_ =
                std::make_shared<const Bytes>(std::move(body));
          }
        });

    timerLoop(scheduler_,
              kHeartbeatInterval,
              weakCb(*this, [](std::shared_ptr<SayHello> &&self) {
                self->onHeartbeat();
              }));

    log()->debug("started");
  }

  SayHello::RequestCtx::RequestCtx(std::chrono::microseconds t) : sent(t) {}

  bool SayHello::TimeAndPeerId::operator<(const TimeAndPeerId &other) const {
    return t < other.t;
  }

  void SayHello::sayHello(const PeerId &peer_id) {
    if (!request_body_) {
      log()->debug("no current head yet");
      return;
    }
    if (active_requests_.count(peer_id) != 0) {
      return;
    }
    auto sent = clock_->nowMicro();
    active_requests_.insert(std::make_pair(peer_id, RequestCtx(sent)));
    active_requests_by_sent_time_.insert({sent, peer_id});

    host_->newStream(libp2p::peer::PeerInfo{peer_id, {}},
                     getProtocolId(),
                     [wptr = weak_from_this(), peer_id](auto rstream) {
                       auto self = wptr.lock();
                       if (self) {
                         if (rstream) {
                           self->onConnected(
                               peer_id,
                               std::make_shared<CborStream>(rstream.value()));
                         } else {
                           log()->debug("cannot connect to {}: {}",
                                        peer_id.toBase58(),
                                        rstream.error().message());
                           self->clearRequest(peer_id);
                         }
                       }
                     });

    log()->debug("saying hello to {}", peer_id.toBase58());
  }

  void SayHello::onConnected(const PeerId &peer_id, StreamPtr stream) {
    auto it = active_requests_.find(peer_id);
    if (it == active_requests_.end()) {
      log()->error("request not found for {}", peer_id.toBase58());
      stream->close();
      return;
    }

    if (!request_body_) {
      log()->error("ignoring {}, no current head", peer_id.toBase58());
      stream->close();
      clearRequest(peer_id);
      return;
    }

    auto &ctx = it->second;
    ctx.stream = std::move(stream);

    ctx.stream->stream()->write(
        *request_body_,
        request_body_->size(),
        [wptr = weak_from_this(), buf = request_body_, peer_id](auto res) {
          auto self = wptr.lock();
          if (self) {
            self->onRequestWritten(peer_id, res);
          }
        });
  }

  void SayHello::onRequestWritten(const PeerId &peer_id,
                                  outcome::result<size_t> result) {
    auto it = active_requests_.find(peer_id);
    if (it == active_requests_.end()) {
      return;
    }

    if (!result) {
      clearRequest(peer_id);
      log()->debug("message write error for peer {}: {}",
                   peer_id.toBase58(),
                   result.error().message());
      return;
    }

    it->second.stream->read<LatencyMessage>(
        [peer_id, self{shared_from_this()}](auto result) {
          self->onResponseRead(peer_id, result);
        });
  }

  void SayHello::onResponseRead(const PeerId &peer_id,
                                outcome::result<LatencyMessage> result) {
    auto it = active_requests_.find(peer_id);
    if (it == active_requests_.end()) {
      return;
    }

    auto time_sent = it->second.sent;

    clearRequest(peer_id);

    if (!result) {
      log()->error("cannot read latency message from peer {}: {}",
                   peer_id.toBase58(),
                   result.error().message());
      return;
    }

    // TODO(artem): do smth with clock results

    uint64_t latency = (clock_->nowMicro() - time_sent).count();

    log()->debug("peer {} latency: {} usec", peer_id.toBase58(), latency);

    events_->signalPeerLatency(
        events::PeerLatency{.peer_id = peer_id, .latency_usec = latency});
  }

  void SayHello::onHeartbeat() {
    auto expire_time = clock_->nowMicro() - kHeartbeatInterval;

    while (!active_requests_by_sent_time_.empty()) {
      auto it = active_requests_by_sent_time_.begin();
      if (it->t > expire_time) {
        break;
      }

      auto peer_id = it->p;
      active_requests_by_sent_time_.erase(it);

      auto it2 = active_requests_.find(peer_id);
      assert(it2 != active_requests_.end());

      it2->second.stream->close();
      active_requests_.erase(it2);

      log()->debug("request timed out for peer {}", peer_id.toBase58());
    }
  }

  void SayHello::clearRequest(const PeerId &peer_id) {
    auto it = active_requests_.find(peer_id);
    if (it == active_requests_.end()) {
      return;
    }
    auto sent = it->second.sent;
    if (it->second.stream) {
      it->second.stream->close();
    }
    active_requests_.erase(it);
    active_requests_by_sent_time_.erase({sent, peer_id});
  }

}  // namespace fc::sync
