/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_POWER_TABLE_IMPL_HPP
#define FILECOIN_CORE_STORAGE_POWER_TABLE_IMPL_HPP

#include "storage/power/power_table.hpp"
namespace fc::storage::power {
  class PowerTableImpl : public PowerTable {
   public:
    outcome::result<int> getMinerPower(
        const primitives::address::Address &address) const override;

    outcome::result<void> setMinerPower(
        const primitives::address::Address &address, int power_amount) override;

    outcome::result<void> removeMiner(
        const primitives::address::Address &address) override;

   private:
    std::unordered_map<std::string, int> power_table_;
  };
}  // namespace fc::storage::power
#endif  // FILECOIN_CORE_STORAGE_POWER_TABLE_IMPL_HPP
