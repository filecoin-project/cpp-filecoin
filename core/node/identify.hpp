/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "node/events_fwd.hpp"

#include <libp2p/event/bus.hpp>
#include <libp2p/peer/peer_info.hpp>

namespace fc::sync {

  class Identify : public std::enable_shared_from_this<Identify> {
   public:
    using PeerId = libp2p::peer::PeerId;

    Identify(
        std::shared_ptr<libp2p::Host> host,
        std::shared_ptr<libp2p::protocol::Identify> identify_protocol,
        std::shared_ptr<libp2p::protocol::IdentifyPush> identify_push_protocol,
        std::shared_ptr<libp2p::protocol::IdentifyDelta>
            identify_delta_protocol);

    void start(std::shared_ptr<events::Events> events);

   private:
    void onIdentifyReceived(const PeerId &peer_id);

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<libp2p::protocol::Identify> identify_protocol_;
    std::shared_ptr<libp2p::protocol::IdentifyPush> identify_push_protocol_;
    std::shared_ptr<libp2p::protocol::IdentifyDelta> identify_delta_protocol_;

    std::shared_ptr<events::Events> events_;
    events::Connection on_identify_;
    libp2p::event::Handle on_disconnect_;
  };

}  // namespace fc::sync
