/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MERKLEDAG_BRIDGE_IMPL_HPP
#define CPP_FILECOIN_MERKLEDAG_BRIDGE_IMPL_HPP

#include "storage/ipfs/graphsync/graphsync.hpp"

namespace fc::storage::ipfs::graphsync {

  class MerkleDagBridgeImpl : public MerkleDagBridge {
   public:
    explicit MerkleDagBridgeImpl(
        std::shared_ptr<merkledag::MerkleDagService> service);

   private:
    outcome::result<size_t> select(
        gsl::span<const uint8_t> root_cid,
        gsl::span<const uint8_t> selector,
        std::function<bool(const common::Buffer &cid,
                           const common::Buffer &data)> handler) const override;

    std::shared_ptr<merkledag::MerkleDagService> service_;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_MERKLEDAG_BRIDGE_IMPL_HPP
