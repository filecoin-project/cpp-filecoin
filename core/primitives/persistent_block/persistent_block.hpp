/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_BLOCK_PERSISTENT_BLOCK_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_BLOCK_PERSISTENT_BLOCK_HPP

#include "storage/ipfs/ipfs_block.hpp"

namespace fc::primitives::blockchain::block {
  class PersistentBlock : public storage::ipfs::IpfsBlock {
   public:
    virtual ~PersistentBlock() override = default;

    PersistentBlock(CID cid, gsl::span<const uint8_t> data);

    const CID &getCID() const override;

    const common::Buffer &getRawBytes() const override;

   private:
    CID cid_;
    common::Buffer data_;
  };
}  // namespace fc::blockchain::block

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_BLOCK_PERSISTENT_BLOCK_HPP
