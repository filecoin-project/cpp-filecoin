/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SECTOR_COUNTER_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SECTOR_COUNTER_HPP

#include "common/outcome.hpp"
#include "primitives/types.hpp"

namespace fc::mining {
  using primitives::SectorNumber;

  class SectorCounter {
   public:
    virtual ~SectorCounter() = default;

    virtual outcome::result<SectorNumber> next() = 0;
  };

}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SECTOR_COUNTER_HPP
