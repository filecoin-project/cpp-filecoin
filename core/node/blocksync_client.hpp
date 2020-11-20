/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BLOCKSYNC_CLIENT_HPP
#define CPP_FILECOIN_SYNC_BLOCKSYNC_CLIENT_HPP

#include <unordered_set>

#include "blocksync_common.hpp"
#include "common/libp2p/cbor_stream.hpp"

namespace fc::sync::blocksync {
  using common::libp2p::CborStream;

  class BlocksyncClient : public std::enable_shared_from_this<BlocksyncClient> {
   public:
    enum class Error {
      BLOCKSYNC_CLIENT_NOT_INITALIZED = 1,
      BLOCKSYNC_FEATURE_NYI,
      BLOCKSYNC_STORE_ERROR_CIDS_MISMATCH,
      BLOCKSYNC_INCONSISTENT_RESPONSE,
      BLOCKSYNC_INCOMPLETE_RESPONSE,
    };

    BlocksyncClient(std::shared_ptr<libp2p::Host> host,
                    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld);

    ~BlocksyncClient();

    void start(std::shared_ptr<events::Events> events);

    outcome::result<void> makeRequest(const PeerId &peer,
                                      std::vector<CID> blocks,
                                      uint64_t depth,
                                      RequestOptions options);

    /// Closes all streams
    void stop();

   private:
    using StreamPtr = std::shared_ptr<CborStream>;

    struct Ctx {
      std::unordered_set<CID> waitlist;
      RequestOptions options;
      StreamPtr stream;
      PeerId peer;
    };

    using Requests = std::unordered_map<uint64_t, Ctx>;

    void onConnected(uint64_t counter,
                     common::Buffer binary_request,
                     outcome::result<StreamPtr> rstream);

    void onRequestWritten(uint64_t counter, outcome::result<size_t> result);

    void onResponseRead(uint64_t counter, outcome::result<Response> result);

    void closeRequest(Requests::iterator it, std::error_code error);

    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld_;
    std::shared_ptr<events::Events> events_;
    bool initialized_ = false;
    uint64_t request_counter_ = 0;
    Requests requests_;
  };

}  // namespace fc::sync::blocksync

OUTCOME_HPP_DECLARE_ERROR(fc::sync::blocksync, BlocksyncClient::Error);

#endif  // CPP_FILECOIN_SYNC_BLOCKSYNC_CLIENT_HPP
