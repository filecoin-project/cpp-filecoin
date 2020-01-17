/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power_table_impl.hpp"

#include "power/power_table_error.hpp"
#include "primitives/address/address_codec.hpp"

fc::outcome::result<fc::primitives::BigInt>
fc::power::PowerTableImpl::getMinerPower(
    const fc::primitives::address::Address &address) const {
  auto result = power_table_.find(primitives::address::encodeToString(address));
  if (result == power_table_.end()) {
    return outcome::failure(fc::power::PowerTableError::NO_SUCH_MINER);
  }

  return result->second;
}

fc::outcome::result<void> fc::power::PowerTableImpl::setMinerPower(
    const fc::primitives::address::Address &address,
    fc::primitives::BigInt power_amount) {
  if (power_amount <= 0) return PowerTableError::NEGATIVE_POWER;
  power_table_[primitives::address::encodeToString(address)] = power_amount;
  return outcome::success();
}

fc::outcome::result<void> fc::power::PowerTableImpl::removeMiner(
    const fc::primitives::address::Address &address) {
  if (power_table_.erase(primitives::address::encodeToString(address)) == 0)
    return PowerTableError::NO_SUCH_MINER;

  return outcome::success();
}

size_t fc::power::PowerTableImpl::getSize() const {
  return power_table_.size();
}

fc::primitives::BigInt fc::power::PowerTableImpl::getMaxPower() const {
  if (power_table_.size() == 0) return 0;

  auto res = std::max(
      power_table_.cbegin(), power_table_.cend(), [&](auto rhs, auto lhs) {
        return rhs->second < lhs->second;
      });

  return res->second;
}

fc::outcome::result<std::vector<fc::primitives::address::Address>>
fc::power::PowerTableImpl::getMiners() const {
  std::vector<primitives::address::Address> result = {};
  for (auto &elem : power_table_) {
    OUTCOME_TRY(miner_addr, primitives::address::decodeFromString(elem.first));
    result.push_back(miner_addr);
  }
  return result;
}
