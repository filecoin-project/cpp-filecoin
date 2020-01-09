/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power_table_impl.hpp"

#include "primitives/address/address_codec.hpp"
#include "storage/power/power_table_error.hpp"

fc::outcome::result<int> fc::storage::power::PowerTableImpl::getMinerPower(
    const fc::primitives::address::Address &address) const {
  auto result = power_table_.find(primitives::address::encodeToString(address));
  if (result == power_table_.end()) {
    return PowerTableError::NO_SUCH_MINER;
  }

  return result->second;
}

fc::outcome::result<void> fc::storage::power::PowerTableImpl::setMinerPower(
    const fc::primitives::address::Address &address, int power_amount) {
  if (power_amount <= 0) return PowerTableError::NEGATIVE_POWER;
  power_table_[primitives::address::encodeToString(address)] = power_amount;
  return outcome::success();
}

fc::outcome::result<void> fc::storage::power::PowerTableImpl::removeMiner(
    const fc::primitives::address::Address &address) {
  if (power_table_.erase(primitives::address::encodeToString(address)) == 0)
    return PowerTableError::NO_SUCH_MINER;

  return outcome::success();
}
