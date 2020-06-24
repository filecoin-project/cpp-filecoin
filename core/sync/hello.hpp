/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_HELLO_HPP
#define CPP_FILECOIN_SYNC_HELLO_HPP

#include <set>
#include <unordered_map>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "clock/utc_clock.hpp"
#include "common/buffer.hpp"
#include "common/libp2p/cbor_stream.hpp"
#include "primitives/big_int.hpp"
#include "primitives/tipset/tipset_key.hpp"

namespace fc::sync {
  using common::libp2p::CborStream;

  class Hello : public std::enable_shared_from_this<Hello> {
   public:
    using PeerId = libp2p::peer::PeerId;
    using BigInt = primitives::BigInt;

    struct Message {
      std::vector<CID> heaviest_tipset;
      uint64_t heaviest_tipset_height;
      BigInt heaviest_tipset_weight;
      CID genesis;
    };

    struct LatencyMessage {
      int64_t arrival, sent;
    };

    /// Callback for incoming hellos
    using HelloFeedback =
        std::function<void(const PeerId &peer, outcome::result<Message>)>;

    /// Callback for latency responses
    using LatencyFeedback =
        std::function<void(const PeerId &peer, outcome::result<uint64_t>)>;

    /// Hello protocol errors
    enum class Error {
      HELLO_NO_CONNECTION = 1,
      HELLO_TIMEOUT,
      HELLO_MALFORMED_MESSAGE,
      HELLO_GENESIS_MISMATCH,

      // internal error for partial data received
      HELLO_INTERNAL_PARTIAL_DATA,
    };

    /// Starts accepting streams
    void start(std::shared_ptr<libp2p::Host> host,
               std::shared_ptr<clock::UTCClock> clock,
               CID genesis_cid,
               const Message &initial_state,
               HelloFeedback hello_feedback,
               LatencyFeedback latency_feedback);

    /// Stops serving the protocol
    void stop();

    void sayHello(const PeerId &peer_id);

    /// Periodic callback to detect timed out requests
    void onHeartbeat();

    void onHeadChanged(Message state);

   private:
    using StreamPtr = std::shared_ptr<CborStream>;
    using SharedBuffer = std::shared_ptr<const common::Buffer>;

    /// Says if protocol started: genesis must not be empty
    bool started() const;

    /// Outbound stream connected
    void onConnected(const PeerId &peer_id, outcome::result<StreamPtr> rstream);

    void onRequestWritten(const PeerId &peer_id,
                          outcome::result<size_t> result);

    void onResponseRead(const PeerId &peer_id,
                        outcome::result<LatencyMessage> result);

    void clearRequest(const PeerId &peer_id);

    /// Inbound stream accepted
    void onAccepted(StreamPtr stream);

    void onRequestRead(const StreamPtr &stream,
                       outcome::result<Message> result);

    std::shared_ptr<libp2p::Host> host_;

    std::shared_ptr<clock::UTCClock> clock_;

    boost::optional<CID> genesis_;

    HelloFeedback hello_feedback_;
    LatencyFeedback latency_feedback_;

    std::vector<CID> current_tipset_;

    /// Cached request body as per current height
    SharedBuffer request_body_;

    /// Info about outgoing hello
    struct RequestCtx {
      StreamPtr stream;
      clock::Time sent;

      explicit RequestCtx(clock::Time t);
    };

    std::unordered_map<PeerId, RequestCtx> active_requests_;

    struct TimeAndPeerId {
      clock::Time t;
      PeerId p;

      bool operator<(const TimeAndPeerId &other) const;
    };

    std::multiset<TimeAndPeerId> active_requests_by_sent_time_;
  };
  CBOR_TUPLE(Hello::Message,
             heaviest_tipset,
             heaviest_tipset_height,
             heaviest_tipset_weight,
             genesis)
  CBOR_TUPLE(Hello::LatencyMessage, arrival, sent)

}  // namespace fc::sync

OUTCOME_HPP_DECLARE_ERROR(fc::sync, Hello::Error);

#endif  // CPP_FILECOIN_SYNC_HELLO_HPP
