/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_SEALING_TYPES_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_SEALING_TYPES_HPP

#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/types.hpp"

namespace fc::sector_storage::sealing {
  using primitives::ChainEpoch;
  using primitives::DealId;

  /**
   * DealSchedule communicates the time interval of a storage deal. The deal
   * must appear in a sealed (proven) sector no later than StartEpoch, otherwise
   * it is invalid.
   */
  struct DealSchedule {
    ChainEpoch start_epoch;
    ChainEpoch end_epoch;
  };

  /** DealInfo is a tuple of deal identity and its schedule */
  struct DealInfo {
    DealId deal_id;
    DealSchedule deal_schedule;
  };
}  // namespace fc::sector_storage::sealing

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_SEALING_TYPES_HPP
