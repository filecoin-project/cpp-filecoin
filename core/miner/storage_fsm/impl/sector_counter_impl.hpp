/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SECTOR_COUNTER_IMPL_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SECTOR_COUNTER_IMPL_HPP

#include "miner/storage_fsm/sector_counter.hpp"

#include "primitives/stored_counter/stored_counter.hpp"

namespace fc::mining {
  using primitives::Datastore;
  using primitives::StoredCounter;

  const std::string kSectorCounterKey = "/storage/nextid";

  class SectorCounterImpl : public SectorCounter {
   public:
    SectorCounterImpl(const std::shared_ptr<Datastore>& datastore);

    outcome::result<SectorNumber> next() override;

   private:
    std::shared_ptr<StoredCounter> counter_;
  };

}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SECTOR_COUNTER_IMPL_HPP
