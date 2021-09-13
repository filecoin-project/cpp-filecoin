/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <future>
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
    using Cb = std::function<void(void)>;

    struct EventWatch {
      Address provider;
      DealId deal_id;
      boost::optional<SectorNumber> sector_number;
      Cb cb;
    };

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
                                       Cb cb) = 0;
  };
}  // namespace fc::markets::storage::chain_events
