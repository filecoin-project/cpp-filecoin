/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pubsub_gate.hpp"

#include "clock/utc_clock.hpp"
#include "codec/cbor/cbor.hpp"
#include "primitives/block/block.hpp"
#include "primitives/cid/cid_of_cbor.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync");
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

  // uint64_t timestamp;
  // uint64_t nonce;
  // std::string node_name;

  PubSubGate::PubSubGate(std::shared_ptr<clock::UTCClock> clock,
                         std::shared_ptr<Gossip> gossip)
      : clock_(std::move(clock)), gossip_(std::move(gossip)) {}

  void PubSubGate::start(const std::string &network_name,
                         const std::string &this_node_name) {
    using GossipMessage = libp2p::protocol::gossip::Gossip::Message;

    blocks_topic_ = std::string("/fil/blocks/") + network_name;
    heads_topic_ = std::string("/fil/headnotifs/") + network_name;
    msgs_topic_ = std::string("/fil/msgs/") + network_name;

#define SUBSCRIBE_TO_TOPIC(WHICH_TOPIC, WHICH_SIGNAL, WHICH_CALLBACK)       \
  gossip_->subscribe(                                                       \
      {WHICH_TOPIC},                                                        \
      [wptr = weak_from_this()](boost::optional<const GossipMessage &> m) { \
        if (!m) {                                                           \
          return;                                                           \
        }                                                                   \
        auto self = wptr.lock();                                            \
        if (!self) {                                                        \
          return;                                                           \
        }                                                                   \
        if (self->WHICH_SIGNAL.empty()) {                                   \
          return;                                                           \
        }                                                                   \
        auto peer_expected = decodeSender(m->from);                         \
        if (!peer_expected) {                                               \
          return;                                                           \
        }                                                                   \
        self->WHICH_CALLBACK(peer_expected.value(), m->data);               \
      })

    blocks_subscription_ =
        SUBSCRIBE_TO_TOPIC(blocks_topic_, blocks_signal_, onBlock);
    heads_subscription_ =
        SUBSCRIBE_TO_TOPIC(heads_topic_, heads_signal_, onHead);
    blocks_subscription_ = SUBSCRIBE_TO_TOPIC(msgs_topic_, msgs_signal_, onMsg);

#undef SUBSCRIBE_TO_TOPIC

    this_node_name_ = this_node_name;
    nonce_ = clockNano();
  }

  void PubSubGate::stop() {
    msgs_subscription_.cancel();
    heads_subscription_.cancel();
    blocks_subscription_.cancel();
    nonce_ = -1;
    this_node_name_.clear();
  }

  PubSubGate::connection_t PubSubGate::subscribeToBlocks(
      const std::function<OnBlockAvailable> &subscriber) {
    return blocks_signal_.connect(subscriber);
  }

  PubSubGate::connection_t PubSubGate::subscribeToHeads(
      const std::function<OnHeadAvailable> &subscriber) {
    return heads_signal_.connect(subscriber);
  }

  PubSubGate::connection_t PubSubGate::subscribeToMessages(
      const std::function<OnMessageAvailable> &subscriber) {
    return msgs_signal_.connect(subscriber);
  }

  outcome::result<void> PubSubGate::publishHead(const Tipset &tipset,
                                                const BigInt &weight) {
    codec::cbor::CborEncodeStream encoder;
    try {
      encoder << (encoder.list()
                  << tipset.key.cids() << tipset.blks << tipset.height()
                  << weight << clockNano() / 1000000 << nonce_
                  << this_node_name_);
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }

    if (!gossip_->publish({heads_topic_}, encoder.data())) {
      return Error::SYNC_PUBSUB_FAILURE;
    }

    return outcome::success();
  }

  outcome::result<void> PubSubGate::publishBlock(
      const BlockHeader &header,
      const std::vector<CID> &bls_msgs,
      const std::vector<CID> &secp_msgs) {
    codec::cbor::CborEncodeStream encoder;
    try {
      encoder << (encoder.list() << header << bls_msgs << secp_msgs);
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }

    if (!gossip_->publish({blocks_topic_}, encoder.data())) {
      return Error::SYNC_PUBSUB_FAILURE;
    }

    return outcome::success();
  }

  bool PubSubGate::started() {
    return (nonce_ != -1ull);
  }

  uint64_t PubSubGate::clockNano() {
    return clock_->nowUTC().unixTimeNano().count();
  }

  void PubSubGate::onBlock(const PeerId &from, const Bytes &raw) {
    try {
      BlockMsg bm;
      codec::cbor::CborDecodeStream decoder(raw);
      decoder.list() >> bm.header >> bm.bls_messages >> bm.secp_messages;

      OUTCOME_EXCEPT(cid,
                     primitives::cid::getCidOfCbor<BlockHeader>(bm.header));

      blocks_signal_(from, cid, bm);
    } catch (std::system_error &e) {
      // TODO peer feedbacks
      log()->error(
          "cannot decode BlockMsg from peer {}, {}", from.toBase58(), e.what());
      return;
    }
  }

  void PubSubGate::onHead(const PeerId &from, const Bytes &raw) {
    std::vector<CID> cids;
    std::vector<BlockHeader> blks;
    uint64_t height;
    uint64_t timestamp;
    uint64_t nonce;
    std::string node_name;

    HeadMsg head_msg;

    try {
      codec::cbor::CborDecodeStream decoder(raw);
      decoder.list() >> cids >> blks >> height >> head_msg.weight >> timestamp
          >> nonce >> node_name;

      OUTCOME_EXCEPT(tipset, Tipset::create(blks));

      // TODO validate cids and other fields

      head_msg.tipset = std::move(tipset);

    } catch (std::system_error &e) {
      // TODO peer feedbacks
      log()->error(
          "cannot decode HeadMsg from peer {}, {}", from.toBase58(), e.what());
      return;
    }

    heads_signal_(from, head_msg);
  }

  void PubSubGate::onMsg(const PeerId &from, const Bytes &raw) {
    constexpr uint8_t kCborTwoElementsArrayHeader = 0x82;

    if (raw.empty()) {
      // TODO peer feedbacks
      log()->error("pubsub: empty message from peer {}", from.toBase58());
      return;
    }

    std::error_code e;

    auto cid_res = common::getCidOf(raw);
    if (!cid_res) {
      e = cid_res.error();
    } else {
      bool decoding_secp_msg = (raw.data()[0] == kCborTwoElementsArrayHeader);

      if (decoding_secp_msg) {
        auto res = codec::cbor::decode<SignedMessage>(raw);
        if (res) {
          msgs_signal_(from,
                       cid_res.value(),
                       common::Buffer(raw),
                       res.value().message,
                       std::cref(res.value().signature));
        } else {
          e = res.error();
        }
      } else {
        auto res = codec::cbor::decode<UnsignedMessage>(raw);
        if (res) {
          msgs_signal_(from,
                       cid_res.value(),
                       common::Buffer(raw),
                       res.value(),
                       boost::none);
        } else {
          e = res.error();
        }
      }
    }

    if (e) {
      // TODO peer feedbacks
      log()->error("pubsub: cannot decode message from peer {}, {}",
                   from.toBase58(),
                   e.message());
    }
  }

}  // namespace fc::sync
