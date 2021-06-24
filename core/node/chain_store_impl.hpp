/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/block_validator/block_validator.hpp"
#include "node/common.hpp"
#include "node/head_constructor.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/chain/chain_store.hpp"

namespace fc::sync {
  using blockchain::block_validator::BlockValidator;
  using primitives::tipset::PutBlockHeader;
  using primitives::tipset::chain::Path;

  class ChainStoreImpl : public fc::storage::blockchain::ChainStore,
                         public std::enable_shared_from_this<ChainStoreImpl> {
   public:
    ChainStoreImpl(std::shared_ptr<storage::ipfs::IpfsDatastore> ipld,
                   TsLoadPtr ts_load,
                   std::shared_ptr<PutBlockHeader> put_block_header,
                   TipsetCPtr head,
                   BigInt weight,
                   std::shared_ptr<BlockValidator> block_validator);

    outcome::result<void> start(std::shared_ptr<events::Events> events);

    outcome::result<void> addBlock(const BlockHeader &block) override;

    TipsetCPtr heaviestTipset() const override;

    connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) override;

    primitives::BigInt getHeaviestWeight() const override;

    void update(const Path &path, const BigInt &weight);

   private:
    HeadConstructor head_constructor_;
    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld_;
    TsLoadPtr ts_load_;
    std::shared_ptr<PutBlockHeader> put_block_header_;
    std::shared_ptr<BlockValidator> block_validator_;

    std::shared_ptr<events::Events> events_;

    TipsetCPtr head_;
    BigInt heaviest_weight_;

    boost::signals2::signal<HeadChangeSignature> head_change_signal_;
  };
}  // namespace fc::sync
