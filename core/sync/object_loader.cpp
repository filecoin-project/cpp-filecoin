/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "object_loader.hpp"

#include "peer_manager.hpp"
#include "primitives/cid/cid_of_cbor.hpp"

namespace fc::sync {

  namespace gs = storage::ipfs::graphsync;

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("sync");
      return logger.get();
    }
  }  // namespace

  ObjectLoader::ObjectLoader(
      std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
      std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
      std::shared_ptr<PubSubGate> pub_sub,
      std::shared_ptr<fc::storage::ipfs::graphsync::Graphsync> graphsync,
      PeerId local_peer_id)
      : scheduler_(std::move(scheduler)),
        ipld_(std::move(ipld)),
        pub_sub_(std::move(pub_sub)),
        graphsync_(std::move(graphsync)),
        local_peer_id_(std::move(local_peer_id)) {
    assert(scheduler_);
    assert(ipld_);
    assert(pub_sub_);
    assert(graphsync_);
  }

  void ObjectLoader::init(OnBlockHeader block_cb,
                          OnMsgMetaAvailable meta_cb,
                          OnMessageAvailable msg_cb) {
    block_cb_ = std::move(block_cb);
    meta_cb_ = std::move(meta_cb);
    msg_cb_ = std::move(msg_cb);
    assert(block_cb_);
    assert(meta_cb_);
    assert(msg_cb_);

    blocks_subscr_ = pub_sub_->subscribeToBlocks(
        [this](const PeerId &from, const CID &cid, const BlockMsg &msg) {
          onBlockFromPubSub(from, cid, msg);
        });

    msgs_subscr_ = pub_sub_->subscribeToMessages(
        [this](const PeerId &from,
               const CID &cid,
               const common::Buffer &raw,
               const UnsignedMessage &msg,
               boost::optional<std::reference_wrapper<const Signature>>
                   signature) {
          onMessageFromPubSub(from, cid, raw, msg, signature);
        });

    graphsync_subscr_ = graphsync_->subscribe(
        [this](const PeerId &from, const CID &cid, const common::Buffer &data) {
          onGraphsyncData(from, cid, data);
        });

    initialized_ = true;
  }

  void ObjectLoader::setDefaultPeer(const PeerId &peer) {
    if (peer != local_peer_id_
        && (!default_peer_.has_value() || peer != default_peer_.value())) {
      default_peer_ = peer;
    }
  }

  outcome::result<void> ObjectLoader::loadObjects(
      const std::vector<ObjectWanted> &objects,
      boost::optional<PeerId> preferred_peer) {
    if (!initialized_) {
      return Error::SYNC_NOT_INITIALIZED;
    }

    if (objects.empty()) {
      // for uniformity
      return outcome::success();
    }

    if (!preferred_peer.has_value() || !default_peer_.has_value()) {
      return Error::SYNC_NO_PEERS;
    }

    const PeerId &peer = preferred_peer.has_value()
                             ? preferred_peer.value()
                             : default_peer_.value();

    for (const auto &wanted : objects) {
      loadObject(wanted.cid, wanted.type, peer);
    }

    return outcome::success();
  }

  void ObjectLoader::loadObject(const CID &cid,
                                ExpectedType type,
                                const PeerId &peer) {
    // TODO policy if expected type differs
    // TODO peer candidates queue if existing doesn't have the object or
    // whatever

    if (requests_.count(cid)) {
      return;
    }

    requests_.insert(
        {cid, CIDRequest{makeGraphsyncRequest(cid, peer), type, peer}});
  }

  libp2p::protocol::Subscription ObjectLoader::makeGraphsyncRequest(
      const CID &cid, const PeerId &peer) {
    auto makeRequestProgressCallback = [this](CID cid) {
      using namespace storage::ipfs::graphsync;

      return [this, cid = std::move(cid)](ResponseStatusCode code,
                                          const std::vector<Extension> &) {
        onGraphsyncResponseProgress(cid, code);
      };
    };

    static const std::vector<storage::ipfs::graphsync::Extension> extensions;

    return graphsync_->makeRequest(peer,
                                   boost::none,
                                   cid,
                                   {},
                                   extensions,
                                   makeRequestProgressCallback(cid));
  }

  void ObjectLoader::onGraphsyncResponseProgress(
      const CID &cid, storage::ipfs::graphsync::ResponseStatusCode status) {
    log()->debug("request progress from graphsync, cid={}, status={}",
                 cid.toString().value(),
                 gs::statusCodeToString(status));

    // TODO if cid not available from this request then try switch to another
    // peer

    requests_.erase(cid);
  }

  void ObjectLoader::onGraphsyncData(const PeerId &from,
                                     const CID &cid,
                                     const common::Buffer &data) {
    if (!initialized_) {
      return;
    }

    log()->debug("data from graphsync, cid={}, peer={}",
                 cid.toString().value(),
                 from.toBase58());

    bool object_is_valid = true;

    auto cid_res = common::getCidOf(data);
    if (!cid_res || cid_res.value() != cid) {
      log()->info("graphsync data and cid={} don't match, peer={}",
                  cid.toString().value(),
                  from.toBase58());
      object_is_valid = false;
    }

    if (object_is_valid) {
      // TODO dont write whatever received into storage

      auto write_res = ipld_->set(cid, data);
      if (!write_res) {
        log()->error(
            "cannot write graphsync data for cid={} into k/v store, {}",
            cid.toString().value(),
            write_res.error().message());
      }
    }

    auto it = requests_.find(cid);
    if (it == requests_.end()) {
      log()->debug("request not found for cid {}", cid.toString().value());
      return;
    }
    auto &req = it->second;

    switch (req.what_to_expect) {
      case BLOCK_HEADER:
        processBlockHeader(cid, data, object_is_valid);
        break;
      case MSG_META:
        processMsgMeta(cid, data, object_is_valid);
        break;
      case BLS_MESSAGE:
        processMessage(cid, data, object_is_valid, false);
        break;
      case SECP_MESSAGE:
        processMessage(cid, data, object_is_valid, true);
        break;
      default:
        break;
    }
  }

  void ObjectLoader::processBlockHeader(const CID &cid,
                                        const common::Buffer &data,
                                        bool object_is_valid) {
    boost::optional<BlockHeader> header;
    if (object_is_valid) {
      auto res = codec::cbor::decode<BlockHeader>(data);
      if (!res) {
        log()->info("data for cid={} is not a block header",
                    cid.toString().value());
        object_is_valid = false;
      } else {
        header = std::move(res.value());
      }
    }

    // TODO block_cb_ returns bool which means the storage needs it

    block_cb_(cid, object_is_valid, std::move(header));
  }

  void ObjectLoader::processMsgMeta(const CID &cid,
                                    const common::Buffer &data,
                                    bool object_is_valid) {
    std::vector<CID> bls_messages;
    std::vector<CID> secp_messages;
    if (object_is_valid) {
      try {
        codec::cbor::CborDecodeStream decoder(data);
        decoder.list() >> bls_messages >> secp_messages;
      } catch (const std::system_error &e) {
        log()->error("cannot decode Message meta for cid={}, {}",
                     cid.toString().value(),
                     e.what());
        object_is_valid = false;
      }
    }

    // TODO meta_cb_ returns bool which means the storage needs it

    meta_cb_(cid, object_is_valid, bls_messages, secp_messages);
  }

  void ObjectLoader::processMessage(const CID &cid,
                                    const common::Buffer &data,
                                    bool object_is_valid,
                                    bool secp_msg_expected) {
    constexpr uint8_t kCborTwoElementsArrayHeader = 0x82;

    if (object_is_valid) {
      // TODO try to decode and decide whether the object should be persisted

      if (secp_msg_expected) {
        object_is_valid =
            (!data.empty() && data[0] == kCborTwoElementsArrayHeader);
      }
    }

    // TODO msg_cb_ returns bool which means the storage needs it

    msg_cb_(cid, secp_msg_expected, object_is_valid);
  }

  void ObjectLoader::onBlockFromPubSub(const PeerId &from,
                                       const CID &cid,
                                       const BlockMsg &msg) {
    // TODO peer feedback on failures

    if (!initialized_) {
      return;
    }

    bool object_is_valid = true;

    log()->debug("BlockMsg from pubsub, cid={}, peer={}",
                 cid.toString().value(),
                 from.toBase58());

    codec::cbor::CborEncodeStream encoder;
    try {
      encoder << (encoder.list() << msg.bls_messages << msg.secp_messages);
    } catch (std::system_error &e) {
      object_is_valid = false;
    }

    if (object_is_valid) {
      auto data = encoder.data();
      auto cid_res = common::getCidOf(data);
      if (!cid_res || cid_res.value() != msg.header.messages) {
        object_is_valid = false;
      } else {
        auto write_res =
            ipld_->set(msg.header.messages, common::Buffer(std::move(data)));

        if (!write_res) {
          log()->error(
              "cannot write msg metadata for block cid={} into k/v store, {}",
              cid.toString().value(),
              write_res.error().message());

          object_is_valid = false;
        }
      }
    }

    block_cb_(cid, object_is_valid, msg.header);

    meta_cb_(msg.header.messages,
             object_is_valid,
             msg.bls_messages,
             msg.secp_messages);
  }

  void ObjectLoader::onMessageFromPubSub(
      const PeerId &from,
      const CID &cid,
      const common::Buffer &raw,
      const UnsignedMessage &msg,
      boost::optional<std::reference_wrapper<const Signature>> signature) {
    // TODO report peer on wrong msg
    // TODO handle decoded objects by cache

    bool object_is_valid = true;
    auto write_res = ipld_->set(cid, raw);
    if (!write_res) {
      log()->error("cannot write message cid={} into k/v store, {}",
                   cid.toString().value(),
                   write_res.error().message());

      object_is_valid = false;
    }

    msg_cb_(cid, signature.has_value(), object_is_valid);
  }

}  // namespace fc::sync
