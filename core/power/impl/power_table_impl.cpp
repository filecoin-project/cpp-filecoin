/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "power/impl/power_table_impl.hpp"

#include "power/power_table_error.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::power {

  outcome::result<Power> PowerTableImpl::getMinerPower(
      const primitives::address::Address &address) const {
    auto result =
        power_table_.find(primitives::address::encodeToString(address));
    if (result == power_table_.end()) {
      return outcome::failure(PowerTableError::NO_SUCH_MINER);
    }

    return result->second;
  }

  outcome::result<void> PowerTableImpl::setMinerPower(
      const primitives::address::Address &address, Power power_amount) {
    if (power_amount < 0) return PowerTableError::NEGATIVE_POWER;
    power_table_[primitives::address::encodeToString(address)] = power_amount;
    return outcome::success();
  }

  outcome::result<void> PowerTableImpl::removeMiner(
      const primitives::address::Address &address) {
    if (power_table_.erase(primitives::address::encodeToString(address)) == 0)
      return PowerTableError::NO_SUCH_MINER;

    return outcome::success();
  }

  fc::outcome::result<size_t> PowerTableImpl::getSize() const {
    return power_table_.size();
  }

  fc::outcome::result<Power> PowerTableImpl::getMaxPower() const {
    if (power_table_.empty()) return 0;

    auto res = std::max(power_table_.cbegin(),
                        power_table_.cend(),
                        [&](const auto &rhs, const auto &lhs) {
                          return rhs->second < lhs->second;
                        });

    return res->second;
  }

  outcome::result<std::vector<primitives::address::Address>>
  PowerTableImpl::getMiners() const {
    std::vector<primitives::address::Address> result;
    result.reserve(power_table_.size());
    for (auto &elem : power_table_) {
      OUTCOME_TRY(miner_addr,
                  primitives::address::decodeFromString(elem.first));
      result.push_back(miner_addr);
    }
    return result;
  }
}  // namespace fc::power
