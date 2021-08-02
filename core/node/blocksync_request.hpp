/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "node/blocksync_common.hpp"

namespace fc::primitives::tipset {
  struct PutBlockHeader;
}  // namespace fc::primitives::tipset

namespace fc::sync::blocksync {
  using primitives::tipset::PutBlockHeader;

  /// Client request to blocksync server
  /// 1) Makes request (with timeout), 2) Saves blocks and messages to Ipld
  class BlocksyncRequest {
   public:
    enum class Error : int {
      kNotImplemented,
      kStoreCidsMismatch,
      kInconsistentResponse,
      kIncompleteResponse,
      kTimeout,
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
      std::vector<CbCid> blocks_requested;

      /// Blocks which are fully available now
      std::vector<BlockHeader> blocks_available;

      /// Their parents, if the request was with depth
      std::vector<TipsetCPtr> parents;

      /// All their meta/messages is also available
      bool messages_stored = false;
    };

    /// Cancels existing request if still active and makes the new one
    static std::shared_ptr<BlocksyncRequest> newRequest(
        libp2p::Host &host,
        libp2p::basic::Scheduler &scheduler,
        IpldPtr ipld,
        std::shared_ptr<PutBlockHeader> put_block_header,
        PeerId peer,
        std::vector<CbCid> blocks,
        uint64_t depth,
        RequestOptions options,
        uint64_t timeoutMsec,
        std::function<void(Result)> callback);

    virtual ~BlocksyncRequest() = default;

    /// Cancels request if any
    virtual void cancel() = 0;
  };

}  // namespace fc::sync::blocksync

OUTCOME_HPP_DECLARE_ERROR(fc::sync::blocksync, BlocksyncRequest::Error);
