/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/pubsub_gate.hpp"

#include "codec/cbor/cbor_codec.hpp"
#include "common/logger.hpp"
#include "primitives/block/block.hpp"
#include "primitives/cid/cid_of_cbor.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("pubsub_gate");
      return logger.get();
    }

    outcome::result<PeerId> decodeSender(const std::vector<uint8_t> &from) {
      auto res = PeerId::fromBytes(from);
      if (!res) {
        log()->error("cannot decode peer id from gossip message");
      }
      return res;
    }
  }  // namespace

  PubSubGate::PubSubGate(std::shared_ptr<Gossip> gossip)
      : gossip_(std::move(gossip)) {
    assert(gossip_);
  }

  void PubSubGate::start(const std::string &network_name,
                         std::shared_ptr<events::Events> events) {
    using GossipMessage = libp2p::protocol::gossip::Gossip::Message;

    events_ = std::move(events);
    assert(events_);

    blocks_topic_ = std::string("/fil/blocks/") + network_name;
    msgs_topic_ = std::string("/fil/msgs/") + network_name;

#define SUBSCRIBE_TO_TOPIC(WHICH_TOPIC, WHICH_CALLBACK)                 \
  gossip_->subscribe({WHICH_TOPIC},                                     \
                     [](boost::optional<const GossipMessage &> m) {     \
                       if (m) {                                         \
                         log()->debug("gossip msg forwarded");          \
                       }                                                \
                     });                                                \
  gossip_->setValidator(                                                \
      WHICH_TOPIC,                                                      \
      [wptr = weak_from_this()](const Bytes &from, const Bytes &data) { \
        auto self = wptr.lock();                                        \
        if (!self) {                                                    \
          return false;                                                 \
        }                                                               \
        auto peer_expected = decodeSender(from);                        \
        if (!peer_expected) {                                           \
          log()->error("cannot decode sender of gossip msg");           \
          return false;                                                 \
        }                                                               \
        return self->WHICH_CALLBACK(peer_expected.value(), data);       \
      })

    blocks_subscription_ = SUBSCRIBE_TO_TOPIC(blocks_topic_, onBlock);
    msgs_subscription_ = SUBSCRIBE_TO_TOPIC(msgs_topic_, onMsg);

#undef SUBSCRIBE_TO_TOPIC

    peer_connected_event_ =
        events_->subscribePeerConnected([this](const events::PeerConnected &e) {
          gossip_->addBootstrapPeer(e.peer_id, boost::none);
        });
  }

  void PubSubGate::stop() {
    msgs_subscription_.cancel();
    blocks_subscription_.cancel();
  }

  outcome::result<void> PubSubGate::publish(const BlockWithCids &block) {
    OUTCOME_TRY(buffer, codec::cbor::encode(block));
    if (!gossip_->publish({blocks_topic_}, buffer.toVector())) {
      log()->warn("cannot publish block");
    }
    return outcome::success();
  }

  bool PubSubGate::onBlock(const PeerId &from, const Bytes &raw) {
    try {
      OUTCOME_EXCEPT(bm, codec::cbor::decode<BlockWithCids>(raw));
      auto cbor{codec::cbor::encode(bm.header).value()};

      // TODO validate

      events_->signalBlockFromPubSub(
          events::BlockFromPubSub{from, CbCid::hash(cbor), std::move(bm)});

      return true;
    } catch (std::system_error &e) {
      // TODO peer feedbacks
      log()->error(
          "cannot decode BlockMsg from peer {}, {}", from.toBase58(), e.what());
    }
    return false;
  }

  bool PubSubGate::onMsg(const PeerId &from, const Bytes &raw) {
    if (raw.empty()) {
      // TODO peer feedbacks
      log()->error("pubsub: empty message from peer {}", from.toBase58());
      return false;
    }

    std::error_code e;

    auto cid_res = common::getCidOf(raw);
    if (!cid_res) {
      e = cid_res.error();
    } else {
      auto res = codec::cbor::decode<primitives::block::SignedMessage>(raw);
      if (res) {
        events_->signalMessageFromPubSub(events::MessageFromPubSub{
            from,
            std::move(cid_res.value()),
            res.value(),
        });
      } else {
        e = res.error();
      }
    }

    if (e) {
      // TODO peer feedbacks
      log()->error("pubsub: cannot decode message from peer {}, {}",
                   from.toBase58(),
                   e.message());
      return false;
    }
    return true;
  }

}  // namespace fc::sync
