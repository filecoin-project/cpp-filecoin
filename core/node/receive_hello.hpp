/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "fwd.hpp"

#include <libp2p/host/host.hpp>

#include "clock/utc_clock.hpp"
#include "common/libp2p/cbor_stream.hpp"
#include "node/hello.hpp"

namespace fc::sync {
  using common::libp2p::CborStream;
  using libp2p::peer::PeerId;

  class ReceiveHello : public std::enable_shared_from_this<ReceiveHello> {
   public:
    ReceiveHello(std::shared_ptr<libp2p::Host> host,
                 std::shared_ptr<clock::UTCClock> clock,
                 CID genesis,
                 std::shared_ptr<events::Events> events);

    void start();

   private:
    using StreamPtr = std::shared_ptr<CborStream>;

    void onRequestRead(const StreamPtr &stream,
                       outcome::result<HelloMessage> result);

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<clock::UTCClock> clock_;
    CID genesis_;
    std::shared_ptr<events::Events> events_;
  };

}  // namespace fc::sync
