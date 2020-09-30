/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>

#include "node/pubsub.hpp"
#include "primitives/block/block.hpp"
#include "vm/message/message.hpp"

namespace fc::pubsub {
  std::shared_ptr<PubSub> PubSub::make(const std::string &network,
                                       std::shared_ptr<Gossip> gossip,
                                       OnBlock on_block,
                                       OnMessage on_message) {
    auto self{std::make_shared<PubSub>()};
    self->on_block = std::move(on_block);
    self->on_message = std::move(on_message);
    self->blocks_topic = "/fil/blocks/" + network;
    self->messages_topic = "/fil/msgs/" + network;
    self->blocks_sub =
        gossip->subscribe({self->blocks_topic}, [self](auto message) {
          if (message) {
            if (auto _peer{PeerId::fromBytes(message->from)}) {
              if (auto _block{
                      codec::cbor::decode<BlockWithCids>(message->data)}) {
                self->on_block(std::move(_peer.value()),
                               std::move(_block.value()));
              }
            }
          }
        });
    self->messages_sub =
        gossip->subscribe({self->messages_topic}, [self](auto message) {
          if (message) {
            if (auto _peer{PeerId::fromBytes(message->from)}) {
              if (auto _message{
                      codec::cbor::decode<SignedMessage>(message->data)}) {
                self->on_message(std::move(_message.value()));
              }
            }
          }
        });
    self->gossip = std::move(gossip);
    return self;
  }

  outcome::result<void> PubSub::publish(const BlockWithCids &block) {
    OUTCOME_TRY(data, codec::cbor::encode(block));
    gossip->publish({blocks_topic}, std::move(data));
    return outcome::success();
  }

  outcome::result<void> PubSub::publish(const SignedMessage &message) {
    OUTCOME_TRY(data, codec::cbor::encode(message));
    gossip->publish({messages_topic}, std::move(data));
    return outcome::success();
  }
}  // namespace fc::pubsub
