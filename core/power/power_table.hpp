/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_POWER_TABLE_HPP
#define FILECOIN_CORE_POWER_TABLE_HPP

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"

namespace fc::power {

  /**
   * @interface Provides an interface to the power table
   */
  class PowerTable {
   public:
    virtual ~PowerTable() = default;

    /**
     * @brief Get the power of a particular miner
     * @param address of the miner
     * @return power of the miner or No such miner error
     */
    virtual outcome::result<fc::primitives::BigInt> getMinerPower(
        const primitives::address::Address &address) const = 0;

    /**
     * @brief Set the power of a particular miner
     * @param address of the miner
     * @param power_amount - amount of power
     * @return success or Negative power error if amount less than 0
     */
    virtual outcome::result<void> setMinerPower(
        const primitives::address::Address &address,
        fc::primitives::BigInt power_amount) = 0;

    /**
     * @brief Remove a miner from power table
     * @param address of the miner
     * @return success or No such miner error
     */
    virtual outcome::result<void> removeMiner(
        const primitives::address::Address &address) = 0;

    virtual size_t getSize() const = 0;

    virtual fc::primitives::BigInt getMaxPower() const = 0;

    virtual outcome::result<std::vector<primitives::address::Address>>
    getMiners() const = 0;
  };
}  // namespace fc::power
#endif  // FILECOIN_CORE_POWER_TABLE_HPP
