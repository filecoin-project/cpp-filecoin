/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_OBJECT_LOADER_HPP
#define CPP_FILECOIN_SYNC_OBJECT_LOADER_HPP

#include <libp2p/protocol/common/scheduler.hpp>

#include "storage/ipfs/datastore.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"

#include "pubsub_gate.hpp"

namespace fc::sync {

  class PubSubGate;

  class ObjectLoader {
   public:
    // If these callbacks return true, the ObjectLoader saves them into
    // datastore, and ignores otherwise

    using OnBlockHeader =
        std::function<bool(const CID &cid,
                           bool object_is_valid,
                           boost::optional<BlockHeader> header)>;

    using OnMsgMetaAvailable =
        std::function<bool(const CID &cid,
                           bool object_is_valid,
                           const std::vector<CID> &bls_messages,
                           const std::vector<CID> &secp_messages)>;

    using OnMessageAvailable = std::function<bool(
        const CID &cid, bool is_secp_message, bool object_is_valid)>;

    enum ExpectedType {
      WHATEVER,
      BLOCK_HEADER,
      MSG_META,
      BLS_MESSAGE,
      SECP_MESSAGE,
    };

    struct ObjectWanted {
      CID cid;
      ExpectedType type = WHATEVER;
    };

    ObjectLoader(
        std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
        std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
        std::shared_ptr<PubSubGate> pub_sub,
        std::shared_ptr<fc::storage::ipfs::graphsync::Graphsync> graphsync,
        PeerId local_peer_id);

    void init(OnBlockHeader block_cb,
              OnMsgMetaAvailable meta_cb,
              OnMessageAvailable msg_cb);

    // TODO callback about peers

    void setDefaultPeer(const PeerId &peer);

    outcome::result<void> loadObjects(const std::vector<ObjectWanted> &objects,
                                      boost::optional<PeerId> preferred_peer);

   private:
    struct CIDRequest {
      libp2p::protocol::Subscription subscription;
      ExpectedType what_to_expect = WHATEVER;
      PeerId peer;
    };

    // TODO use flat sets/hash maps for performance
    using Requests = std::map<CID, CIDRequest>;

    void loadObject(const CID &cid, ExpectedType type, const PeerId &peer);

    libp2p::protocol::Subscription makeGraphsyncRequest(const CID &cid,
                                                        const PeerId &peer);

    void onGraphsyncResponseProgress(
        const CID &cid, storage::ipfs::graphsync::ResponseStatusCode status);

    void onGraphsyncData(const PeerId &from,
                         const CID &cid,
                         const common::Buffer &data);

    void processBlockHeader(const CID &cid,
                            const common::Buffer &data,
                            bool object_is_valid);

    void processMsgMeta(const CID &cid,
                        const common::Buffer &data,
                        bool object_is_valid);

    void processMessage(const CID &cid,
                        const common::Buffer &data,
                        bool object_is_valid,
                        bool secp_msg_expected);

    void onBlockFromPubSub(const PeerId &from,
                           const CID &cid,
                           const BlockMsg &msg);

    void onMessageFromPubSub(
        const PeerId &from,
        const CID &cid,
        const common::Buffer &raw,
        const UnsignedMessage &msg,
        boost::optional<std::reference_wrapper<const Signature>> signature);

    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld_;
    std::shared_ptr<PubSubGate> pub_sub_;
    std::shared_ptr<storage::ipfs::graphsync::Graphsync> graphsync_;
    const PeerId local_peer_id_;
    OnBlockHeader block_cb_;
    OnMsgMetaAvailable meta_cb_;
    OnMessageAvailable msg_cb_;
    PubSubGate::connection_t blocks_subscr_;
    PubSubGate::connection_t msgs_subscr_;
    storage::ipfs::graphsync::Graphsync::DataConnection graphsync_subscr_;
    bool initialized_ = false;
    boost::optional<PeerId> default_peer_;
    boost::optional<PeerId> last_received_from_;
    Requests requests_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_OBJECT_LOADER_HPP
