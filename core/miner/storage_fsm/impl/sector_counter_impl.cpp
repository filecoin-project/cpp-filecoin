/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/sector_counter_impl.hpp"

namespace fc::mining {
  SectorCounterImpl::SectorCounterImpl(
      const std::shared_ptr<Datastore> &datastore) {
    counter_ = std::make_shared<StoredCounter>(datastore, kSectorCounterKey);
  }

  outcome::result<SectorNumber> SectorCounterImpl::next() {
    return counter_->next();
  }
}  // namespace fc::mining
