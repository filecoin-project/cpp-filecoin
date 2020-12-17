/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BLOCKSYNC_REQUEST_HPP
#define CPP_FILECOIN_SYNC_BLOCKSYNC_REQUEST_HPP

#include "blocksync_common.hpp"

namespace fc::sync::blocksync {

  class BlocksyncRequest {
   public:
    enum class Error : int {
      BLOCKSYNC_FEATURE_NYI,
      BLOCKSYNC_STORE_ERROR_CIDS_MISMATCH,
      BLOCKSYNC_INCONSISTENT_RESPONSE,
      BLOCKSYNC_INCOMPLETE_RESPONSE,
      BLOCKSYNC_TIMEOUT,
    };

    /// Blocks downloaded but may be not yet indexed
    struct Result {
      /// Peer they came from
      boost::optional<PeerId> from;

      /// Not 0 if error occured
      std::error_code error;

      /// Change rating for this peer
      int64_t delta_rating = 0;

      /// Blocks which were requested
      std::vector<CID> blocks_requested;

      /// Blocks which are fully available now
      std::vector<BlockHeader> blocks_available;

      /// Their parents, if the request was with depth
      std::vector<TipsetCPtr> parents;

      /// All their meta/messages is also available
      bool messages_stored = false;
    };

    ~BlocksyncRequest();

    /// Cancels existing request if still active and makes the new one
    void newRequest(libp2p::Host &host,
                     libp2p::protocol::Scheduler &scheduler,
                     Ipld &ipld,
                     PeerId peer,
                     std::vector<CID> blocks,
                     uint64_t depth,
                     RequestOptions options,
                     uint64_t timeoutMsec,
                     std::function<void(Result)> callback);

    /// Cancels request if any
    void cancel();

   private:
    class Impl;
    std::shared_ptr<Impl> impl_;
  };

}  // namespace fc::sync::blocksync

OUTCOME_HPP_DECLARE_ERROR(fc::sync::blocksync, BlocksyncRequest::Error);

#endif  // CPP_FILECOIN_SYNC_BLOCKSYNC_REQUEST_HPP
