/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_SAY_HELLO_HPP
#define CPP_FILECOIN_SYNC_SAY_HELLO_HPP

#include "fwd.hpp"

#include <set>
#include <unordered_map>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/common/scheduler.hpp>

#include "clock/utc_clock.hpp"
#include "common/buffer.hpp"
#include "common/libp2p/cbor_stream.hpp"
#include "hello.hpp"

namespace fc::sync {
  using common::libp2p::CborStream;

  class SayHello : public std::enable_shared_from_this<SayHello> {
   public:
    using PeerId = libp2p::peer::PeerId;

    SayHello(std::shared_ptr<libp2p::Host> host,
             std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
             std::shared_ptr<clock::UTCClock> clock);

    void start(CID genesis, std::shared_ptr<events::Events> events);

   private:
    void sayHello(const PeerId &peer_id);

    /// Periodic callback to detect timed out requests
    void onHeartbeat();

    using StreamPtr = std::shared_ptr<CborStream>;
    using SharedBuffer = std::shared_ptr<const common::Buffer>;

    /// Outbound stream connected
    void onConnected(const PeerId &peer_id, StreamPtr stream);

    void onRequestWritten(const PeerId &peer_id,
                          outcome::result<size_t> result);

    void onResponseRead(const PeerId &peer_id,
                        outcome::result<LatencyMessage> result);

    void clearRequest(const PeerId &peer_id);

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<clock::UTCClock> clock_;

    boost::optional<CID> genesis_;
    std::shared_ptr<events::Events> events_;
    events::Connection peer_connected_event_;
    events::Connection current_head_event_;

    /// Cached request body as per current head
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

    libp2p::protocol::scheduler::Handle heartbeat_handle_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_SAY_HELLO_HPP
