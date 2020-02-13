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

  using primitives::address::Address;
  using Power = primitives::UBigInt;

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
    virtual outcome::result<Power> getMinerPower(
        const Address &address) const = 0;

    /**
     * @brief Set the power of a particular miner
     * @param address of the miner
     * @param power_amount - amount of power
     * @return success or Negative power error if amount less than 0
     */
    virtual outcome::result<void> setMinerPower(const Address &address,
                                                Power power_amount) = 0;

    /**
     * @brief Remove a miner from power table
     * @param address of the miner
     * @return success or No such miner error
     */
    virtual outcome::result<void> removeMiner(const Address &address) = 0;

    /**
     * @brief Get size of table
     * @return number of miners
     */
    virtual fc::outcome::result<size_t> getSize() const = 0;

    /**
     * @brief Get Max power from table
     * @return max power from all miners
     */
    virtual fc::outcome::result<Power> getMaxPower() const = 0;

    /**
     * @brief Get list of all miners from table
     * @return list of miners
     */
    virtual outcome::result<std::vector<Address>> getMiners() const = 0;
  };
}  // namespace fc::power
#endif  // FILECOIN_CORE_POWER_TABLE_HPP
