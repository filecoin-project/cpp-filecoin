/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>
#include "power/power_table.hpp"

namespace fc::power {

  class PowerTableImpl : public PowerTable {
   public:
    outcome::result<Power> getMinerPower(
        const primitives::address::Address &address) const override;

    outcome::result<void> setMinerPower(
        const primitives::address::Address &address,
        Power power_amount) override;

    outcome::result<void> removeMiner(
        const primitives::address::Address &address) override;

    fc::outcome::result<size_t> getSize() const override;

    fc::outcome::result<Power> getMaxPower() const override;

    outcome::result<std::vector<primitives::address::Address>> getMiners()
        const override;

   private:
    std::unordered_map<std::string, Power> power_table_;
  };

}  // namespace fc::power
