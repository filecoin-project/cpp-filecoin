/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BLOCKSYNC_CLIENT_HPP
#define CPP_FILECOIN_SYNC_BLOCKSYNC_CLIENT_HPP

#include "blocksync_common.hpp"

#include "common/libp2p/cbor_stream.hpp"

namespace libp2p {
  class Host;
}

namespace fc::sync::blocksync {
  using common::libp2p::CborStream;

  class BlocksyncClient : public std::enable_shared_from_this<BlocksyncClient> {
   public:
    BlocksyncClient(std::shared_ptr<libp2p::Host> host,
                    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld);

    // simplest peer feedback at the moment
    using OnPeerFeedback =
        std::function<void(const PeerId &peer, std::error_code error)>;

    void init(OnBlockStored block_cb, OnPeerFeedback peer_cb);

    outcome::result<void> makeRequest(const PeerId &peer,
                                      std::vector<CID> blocks,
                                      uint64_t depth,
                                      RequestOptions options);

    /// Closes all streams
    void stop();

   private:
    using StreamPtr = std::shared_ptr<CborStream>;

    struct Ctx {
      Request request;
      StreamPtr stream;
      PeerId peer;
    };

    using Requests = std::unordered_map<uint64_t, Ctx>;

    void onConnected(uint64_t counter, outcome::result<StreamPtr> rstream);

    void onRequestWritten(uint64_t counter, outcome::result<size_t> result);

    void onResponseRead(uint64_t counter, outcome::result<Response> result);

    void closeRequest(Requests::iterator it, std::error_code error);

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld_;

    OnBlockStored block_cb_;
    OnPeerFeedback peer_cb_;
    bool initialized_ = false;
    uint64_t request_counter_ = 0;
    Requests requests_;
  };

}  // namespace fc::sync::blocksync

#endif  // CPP_FILECOIN_SYNC_BLOCKSYNC_CLIENT_HPP
