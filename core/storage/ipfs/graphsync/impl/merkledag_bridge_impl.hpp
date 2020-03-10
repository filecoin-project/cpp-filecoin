/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MERKLEDAG_BRIDGE_IMPL_HPP
#define CPP_FILECOIN_MERKLEDAG_BRIDGE_IMPL_HPP

#include "storage/ipfs/graphsync/graphsync.hpp"

namespace fc::storage::ipfs::graphsync {

  /// Default implementation og Graphsync->MerkleDAG bridge
  class MerkleDagBridgeImpl : public MerkleDagBridge {
   public:
    /// Ctor. called form  MerkleDagBridge::create(...)
    explicit MerkleDagBridgeImpl(
        std::shared_ptr<merkledag::MerkleDagService> service);

   private:
    // overrides MerkleDagBridge interface
    outcome::result<size_t> select(
        const CID &root_cid,
        gsl::span<const uint8_t> selector,
        std::function<bool(const CID &cid, const common::Buffer &data)> handler)
        const override;

    /// MerkleDAG service
    std::shared_ptr<merkledag::MerkleDagService> service_;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_MERKLEDAG_BRIDGE_IMPL_HPP
