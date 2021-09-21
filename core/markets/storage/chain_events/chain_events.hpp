/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/mpool/mpool.hpp"

namespace fc::markets::storage::chain_events {
  using fc::storage::mpool::MpoolUpdate;
  using primitives::DealId;
  using primitives::SectorNumber;
  using primitives::address::Address;

  /**
   * Watches for a specified method on an actor is called
   */
  class ChainEvents {
   public:
    using CommitCb = std::function<void(outcome::result<void>)>;

    virtual ~ChainEvents() = default;

    /**
     * Returnes promise with result when miner actor DealSectorCommitted is
     * called
     * @param provider - provider address
     * @param deal_id - id of committed deal
     * @param cb - callback
     */
    virtual void onDealSectorCommitted(const Address &provider,
                                       const DealId &deal_id,
                                       CommitCb cb) = 0;
  };
}  // namespace fc::markets::storage::chain_events
