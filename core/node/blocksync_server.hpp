/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BLOCKSYNC_SERVER_HPP
#define CPP_FILECOIN_SYNC_BLOCKSYNC_SERVER_HPP

#include "blocksync_common.hpp"

#include "common/libp2p/cbor_stream.hpp"

namespace libp2p {
  struct Host;
}

namespace fc::sync::blocksync {

  /// Serves blocksync protocol
  class BlocksyncServer : public std::enable_shared_from_this<BlocksyncServer> {
   public:
    BlocksyncServer(std::shared_ptr<libp2p::Host> host,
                    TsLoadPtr ts_load,
                    IpldPtr ipld);

    void start();

    void stop();

   private:
    using StreamPtr = std::shared_ptr<common::libp2p::CborStream>;

    void onRequest(StreamPtr stream, outcome::result<Request> request);

    std::shared_ptr<libp2p::Host> host_;
    TsLoadPtr ts_load_;
    IpldPtr ipld_;
    bool started_ = false;
  };

}  // namespace fc::sync::blocksync

#endif  // CPP_FILECOIN_SYNC_BLOCKSYNC_SERVER_HPP
