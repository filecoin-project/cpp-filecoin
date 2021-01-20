/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/protocol/common/subscription.hpp>
#include <string>

#include "common/outcome.hpp"
#include "fwd.hpp"

namespace fc::pubsub {
  using libp2p::peer::PeerId;
  using libp2p::protocol::Subscription;
  using libp2p::protocol::gossip::Gossip;
  using primitives::block::BlockWithCids;
  using vm::message::SignedMessage;

  struct PubSub : std::enable_shared_from_this<PubSub> {
    using OnBlock = std::function<void(PeerId &&, BlockWithCids &&)>;
    using OnMessage = std::function<void(SignedMessage &&)>;

    static std::shared_ptr<PubSub> make(const std::string &network,
                                        std::shared_ptr<Gossip> gossip,
                                        OnBlock on_block,
                                        OnMessage on_message);
    outcome::result<void> publish(const BlockWithCids &block);
    outcome::result<void> publish(const SignedMessage &message);

    OnBlock on_block;
    OnMessage on_message;
    std::string blocks_topic, messages_topic;
    Subscription blocks_sub, messages_sub;
    std::shared_ptr<Gossip> gossip;
  };
}  // namespace fc::pubsub
