/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_POWER_TABLE_IMPL_HPP
#define FILECOIN_CORE_STORAGE_POWER_TABLE_IMPL_HPP

#include "power/power_table.hpp"

namespace fc::power {
  class PowerTableImpl : public PowerTable {
   public:
    outcome::result<fc::primitives::BigInt> getMinerPower(
        const primitives::address::Address &address) const override;

    outcome::result<void> setMinerPower(
        const primitives::address::Address &address,
        fc::primitives::BigInt power_amount) override;

    outcome::result<void> removeMiner(
        const primitives::address::Address &address) override;

    size_t getSize() const override;

    fc::primitives::BigInt getMaxPower() const override;

    outcome::result<std::vector<primitives::address::Address>> getMiners()
        const override;

   private:
    std::unordered_map<std::string, fc::primitives::BigInt> power_table_;
  };
}  // namespace fc::power
#endif  // FILECOIN_CORE_STORAGE_POWER_TABLE_IMPL_HPP
