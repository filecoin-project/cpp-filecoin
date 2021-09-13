/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/signals2.hpp>
#include "primitives/tipset/tipset.hpp"

namespace fc::storage::blockchain {
  using primitives::block::BlockHeader;
  using primitives::tipset::HeadChange;
  using primitives::tipset::HeadChangeType;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::TipsetHash;
  using primitives::tipset::TipsetKey;

  enum class ChainStoreError : int {
    kStoreNotInitialized = 1,
    kNoHeaviestTipset,
    kNoTipsetAtHeight,
    kBlockRejected,
    kIllegalState,
  };

  class ChainStore {
   public:
    virtual ~ChainStore() = default;

    virtual outcome::result<void> addBlock(const BlockHeader &block) = 0;

    virtual TipsetCPtr heaviestTipset() const = 0;

    using connection_t = boost::signals2::connection;
    using HeadChangeSignature = void(const std::vector<HeadChange> &);

    virtual connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) = 0;

    virtual primitives::BigInt getHeaviestWeight() const = 0;
  };

}  // namespace fc::storage::blockchain

OUTCOME_HPP_DECLARE_ERROR(fc::storage::blockchain, ChainStoreError);
