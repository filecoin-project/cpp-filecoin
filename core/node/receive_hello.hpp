/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_RECEIVE_HELLO_HPP
#define CPP_FILECOIN_SYNC_RECEIVE_HELLO_HPP

#include "fwd.hpp"

#include <libp2p/host/host.hpp>

#include "clock/utc_clock.hpp"
#include "common/libp2p/cbor_stream.hpp"

#include "hello.hpp"

namespace fc::sync {
  using common::libp2p::CborStream;

  class ReceiveHello : public std::enable_shared_from_this<ReceiveHello> {
   public:
    using PeerId = libp2p::peer::PeerId;

    ReceiveHello(std::shared_ptr<libp2p::Host> host,
                 std::shared_ptr<clock::UTCClock> clock);

    void start(CID genesis, std::shared_ptr<events::Events> events);

   private:
    using StreamPtr = std::shared_ptr<CborStream>;

    void onRequestRead(const StreamPtr &stream,
                       outcome::result<HelloMessage> result);

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<clock::UTCClock> clock_;
    boost::optional<CID> genesis_;
    std::shared_ptr<events::Events> events_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_RECEIVE_HELLO_HPP
