/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "receive_hello.hpp"

#include <cassert>

#include "common/logger.hpp"
#include "events.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("hello");
      return logger.get();
    }
  }  // namespace

  ReceiveHello::ReceiveHello(std::shared_ptr<libp2p::Host> host,
                             std::shared_ptr<clock::UTCClock> clock)
      : host_(std::move(host)), clock_(std::move(clock)) {
    assert(host_);
    assert(clock_);
  }

  void ReceiveHello::start(CID genesis,
                           std::shared_ptr<events::Events> events) {
    genesis_ = std::move(genesis);
    events_ = std::move(events);
    assert(events_);

    host_->setProtocolHandler(
        kHelloProtocol,
        [wptr =
             weak_from_this()](std::shared_ptr<libp2p::connection::Stream> s) {
          assert(s && !s->isClosedForRead());

          auto self = wptr.lock();
          if (self) {
            auto cbor_stream = std::make_shared<CborStream>(s);
            cbor_stream->read<HelloMessage>(
                [cbor_stream, wptr](outcome::result<HelloMessage> result) {
                  auto self = wptr.lock();
                  if (self) {
                    self->onRequestRead(cbor_stream, result);
                  } else {
                    cbor_stream->close();
                  }
                });
          } else {
            s->reset();
          }
        });

    log()->debug("started");
  }

  void ReceiveHello::onRequestRead(const StreamPtr &stream,
                                   outcome::result<HelloMessage> result) {
    if (!genesis_) {
      stream->close();
      return;
    }

    auto peer_res = stream->stream()->remotePeerId();
    if (!peer_res) {
      log()->error("no remote peer");
      stream->close();
      return;
    }

    if (!result) {
      log()->error("request read failed: {}", result.error().message());
      stream->close();
      return;
    }

    auto& msg = result.value();

    if (msg.genesis != genesis_.value()) {
      log()->error("peer {} has another genesis: {}",
                   peer_res.value().toBase58(),
                   msg.genesis.toString().value());
      stream->close();
      return;
    }

    auto arrival = clock_->microsecSinceEpoch();

    events_->signalTipsetFromHello(events::TipsetFromHello {
        .peer_id = std::move(peer_res.value()),
        .tipset = std::move(msg.heaviest_tipset),
        .height = msg.heaviest_tipset_height,
        .weight = std::move(msg.heaviest_tipset_weight)
    });

    auto sent = clock_->microsecSinceEpoch();

    stream->write(LatencyMessage{arrival * 1000, sent * 1000},
                  [stream](auto) { stream->close(); });
  }

}  // namespace fc::sync
