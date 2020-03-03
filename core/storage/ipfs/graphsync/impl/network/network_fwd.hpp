/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_NETWORK_FWD_HPP
#define CPP_FILECOIN_GRAPHSYNC_NETWORK_FWD_HPP

#include "marshalling/message.hpp"

namespace libp2p::connection {
  class Stream;
}

namespace fc::storage::ipfs::graphsync {

  using StreamPtr = std::shared_ptr<libp2p::connection::Stream>;

  class PeerContext;
  using PeerContextPtr = std::shared_ptr<PeerContext>;

  class PeerToGraphsyncFeedback {
   public:
    virtual ~PeerToGraphsyncFeedback() = default;

    virtual void onBlock(const PeerId &from, CID cid, common::Buffer data) = 0;

    virtual void onRemoteRequest(const PeerId &from,
                                 Message::Request request) = 0;

    virtual void onResponse(const PeerId &peer,
                            RequestId request_id,
                            ResponseStatusCode status,
                            ResponseMetadata metadata) = 0;
  };

  class PeerToNetworkFeedback {
   public:
    virtual ~PeerToNetworkFeedback() = default;

    virtual void canClosePeer(const PeerId &peer) = 0;

    //virtual void closeLocalRequest()
  };

  class EndpointToPeerFeedback {
   public:
    virtual ~EndpointToPeerFeedback() = default;

    virtual void onReaderEvent(const StreamPtr& stream,
                               outcome::result<Message>) = 0;

    virtual void onWriterEvent(const StreamPtr& stream,
                               outcome::result<void> result) = 0;
  };

  /// Operators needed to find a ctx in a set by peer id
  bool operator<(const PeerContextPtr &ctx, const PeerId &peer);
  bool operator<(const PeerId &peer, const PeerContextPtr &ctx);
  bool operator<(const PeerContextPtr &a, const PeerContextPtr &b);

  constexpr std::string_view kProtocolVersion = "/ipfs/graphsync/1.0.0";

  //TODO(artem) sync with go-graphsync
  constexpr size_t kMaxMessageSize = 16 * 1024 * 1024;
  constexpr size_t kMaxPendingBytes = 64 * 1024 * 1024;



}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_NETWORK_FWD_HPP
