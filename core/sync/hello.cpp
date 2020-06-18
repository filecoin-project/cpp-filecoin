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

    constexpr size_t kLatencyBufsize = 32;
    constexpr size_t kMaxLatencyMessageSize = 32;
    constexpr size_t kHelloBufsize = 8192;
    constexpr size_t kMaxHelloMessageSize = kHelloBufsize * 32;

    std::shared_ptr<const common::Buffer> encodeResponse(uint64_t arrival,
                                                         uint64_t sent) {
      using codec::cbor::CborEncodeStream;

      auto ls = CborEncodeStream::list();
      ls << arrival;
      ls << sent;
      CborEncodeStream encoder;
      encoder << ls;

      return std::make_shared<const common::Buffer>(encoder.data());
    }

    outcome::result<std::pair<uint64_t, uint64_t>> decodeResponse(
        const std::vector<uint8_t> &buf, size_t size) {
      using codec::cbor::CborDecodeStream;

      try {
        CborDecodeStream decoder(
            gsl::span<const uint8_t>(buf.data(), buf.data() + size));

        if (!decoder.isList() || decoder.listLength() != 2) {
          return Hello::Error::HELLO_MALFORMED_MESSAGE;
        }

        boost::optional<CborDecodeStream> ls;
        try {
          ls = decoder.list();
        } catch (const std::exception &e) {
          return Hello::Error::HELLO_INTERNAL_PARTIAL_DATA;
        }

        if (!decoder.isEOF()) {
          return Hello::Error::HELLO_MALFORMED_MESSAGE;
        }

        std::pair<uint64_t, uint64_t> ret;
        *ls >> ret.first;
        *ls >> ret.second;

        //return std::move(ret); redundant move
        return ret;

      } catch (const std::exception &e) {
        log()->error("decode hello response error: {}", e.what());
      }

      return Hello::Error::HELLO_MALFORMED_MESSAGE;
    }

    outcome::result<std::pair<Hello::Message, CID>> decodeRequest(
        const std::vector<uint8_t> &buf, size_t size) {
      using fc::codec::cbor::CborDecodeStream;

      try {
        CborDecodeStream decoder(
            gsl::span<const uint8_t>(buf.data(), buf.data() + size));

        if (!decoder.isList() || decoder.listLength() != 4) {
          return Hello::Error::HELLO_MALFORMED_MESSAGE;
        }

        boost::optional<CborDecodeStream> ls;
        try {
          ls = decoder.list();
        } catch (const std::exception &e) {
          return Hello::Error::HELLO_INTERNAL_PARTIAL_DATA;
        }

        if (!decoder.isEOF()) {
          return Hello::Error::HELLO_MALFORMED_MESSAGE;
        }

        std::pair<Hello::Message, CID> ret;

        *ls >> ret.first.heaviest_tipset;
        *ls >> ret.first.heaviest_tipset_height;
        *ls >> ret.first.heaviest_tipset_weight;
        *ls >> ret.second;

        //return std::move(ret);
        return ret;

      } catch (const std::exception &e) {
        log()->error("decode hello request error: {}", e.what());
      }

      return Hello::Error::HELLO_MALFORMED_MESSAGE;
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
        if (self) { self->onAccepted(std::move(rstream)); }
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
        req.second.stream->reset();
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
          if (self) {  self->onConnected(peer_id, std::move(rstream)); }
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

      it2->second.stream->reset();
      active_requests_.erase(it2);

      latency_feedback_(peer_id, Error::HELLO_TIMEOUT);
    }
  }

  void Hello::onHeadChanged(const Message &state) {
    if (current_tipset_ == state.heaviest_tipset) {
      // request body is cached and didnt change
      return;
    }

    current_tipset_ = state.heaviest_tipset;

    using codec::cbor::CborEncodeStream;

    auto ls = CborEncodeStream::list();
    CborEncodeStream cids;
    cids << state.heaviest_tipset;
    ls << cids;
    ls << state.heaviest_tipset_height;
    ls << state.heaviest_tipset_weight;
    ls << genesis_.value();
    CborEncodeStream encoder;
    encoder << ls;

    request_body_ = std::make_shared<const common::Buffer>(encoder.data());
  }

  void Hello::onConnected(const PeerId &peer_id,
                          outcome::result<Hello::StreamPtr> rstream) {
    if (!rstream || rstream.value()->isClosed()) {
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
    rstream.value()->write(
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

    readResponse(peer_id, it->second.stream);
  }

  void Hello::readResponse(const PeerId &peer_id,
                           const StreamPtr &stream) {
    std::vector<uint8_t> read_buf;
    read_buf.resize(kLatencyBufsize);
    gsl::span<uint8_t> span(read_buf.data(), kLatencyBufsize);

    // clang-format off
    stream->readSome(
        span, kLatencyBufsize,
        [wptr = weak_from_this(), buf=std::move(read_buf), peer_id]
        (outcome::result<size_t> res) {
          auto self = wptr.lock();
          if (self) {  self->onResponseRead(peer_id, res, buf); }
        }
    );
    // clang-format on
  }

  void Hello::onResponseRead(const PeerId &peer_id,
                             outcome::result<size_t> result,
                             const std::vector<uint8_t> &read_buf) {
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
      latency_feedback_(peer_id, Error::HELLO_NO_CONNECTION);
      return;
    }

    outcome::result<std::pair<uint64_t, uint64_t>> decode_res{0, 0};
    bool bufs_joined = false;

    auto &ctx = it->second;
    if (ctx.read_buf.empty()) {
      decode_res = decodeResponse(read_buf, result.value());
    } else {
      if (ctx.read_buf.size() + read_buf.size() > kMaxLatencyMessageSize) {
        clearRequest(peer_id);
        latency_feedback_(peer_id, Error::HELLO_MALFORMED_MESSAGE);
        return;
      }

      ctx.read_buf.insert(ctx.read_buf.end(),
                          read_buf.begin(),
                          read_buf.begin() + result.value());
      bufs_joined = true;
      decode_res = decodeResponse(ctx.read_buf, ctx.read_buf.size());
    }

    if (!decode_res) {
      if (static_cast<Error>(decode_res.error().value())
          == Error::HELLO_INTERNAL_PARTIAL_DATA) {
        // partial bytes read
        if (!bufs_joined) {
          ctx.read_buf = read_buf;
        }
        readResponse(peer_id, ctx.stream);
      } else {
        clearRequest(peer_id);
        latency_feedback_(peer_id, Error::HELLO_MALFORMED_MESSAGE);
      }
      return;
    }

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
      it->second.stream->reset();
    }
    active_requests_.erase(it);
    active_requests_by_sent_time_.erase({sent, peer_id});
  }

  void Hello::onAccepted(outcome::result<StreamPtr> rstream) {
    if (!rstream) {
      return;
    }
    std::vector<uint8_t> read_buf;
    read_buf.resize(kHelloBufsize);
    readRequest(rstream.value(), read_buf, 0);
  }

  void Hello::readRequest(const StreamPtr &stream,
                          std::vector<uint8_t> &read_buf,
                          size_t offset) {
    assert(read_buf.size() > offset);
    assert(stream->remotePeerId());

    size_t read_size = read_buf.size() - offset;
    gsl::span<uint8_t> span(read_buf.data() + offset, read_size);

    stream->readSome(
        span,
        read_size,
        [wptr = weak_from_this(), buf = std::move(read_buf), offset, stream](
            outcome::result<size_t> res) mutable {
          auto self = wptr.lock();
          if (self) {
            self->onRequestRead(stream, buf, offset, res);
          }
        });
  }

  void Hello::onRequestRead(const StreamPtr &stream,
                            std::vector<uint8_t> &read_buf,
                            size_t offset,
                            outcome::result<size_t> result) {
    if (!started()) {
      return;
    }

    if (!result) {
      log()->error("Hello request read failed: {}", result.error().message());
      return;
    }

    auto peer_res = stream->remotePeerId();
    if (!peer_res) {
      log()->error("Hello request: no remote peer");
      return;
    }

    size_t bytes_read = offset + result.value();

    assert(bytes_read <= read_buf.size());

    auto decode_res = decodeRequest(read_buf, bytes_read);

    if (!decode_res) {
      if (static_cast<Error>(decode_res.error().value())
          == Error::HELLO_INTERNAL_PARTIAL_DATA) {
        if (bytes_read < kMaxHelloMessageSize) {
          read_buf.resize(read_buf.size() + kHelloBufsize);
          readRequest(stream, read_buf, bytes_read);
          return;
        } else {
          decode_res.error() = Error::HELLO_MALFORMED_MESSAGE;
        }
      }
      hello_feedback_(peer_res.value(), decode_res.error());
      stream->reset();
      return;
    }

    if (decode_res.value().second != genesis_) {
      hello_feedback_(peer_res.value(), Error::HELLO_GENESIS_MISMATCH);
      stream->reset();
      return;
    }

    auto arrival = clock_->nowUTC().unixTimeNano().count();
    hello_feedback_(peer_res.value(), std::move(decode_res.value().first));

    auto sent = clock_->nowUTC().unixTimeNano().count();
    auto response_buf = encodeResponse(arrival, sent);

    stream->write(*response_buf,
                  response_buf->size(),
                  [buf = response_buf, stream](auto) { stream->reset(); });
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
