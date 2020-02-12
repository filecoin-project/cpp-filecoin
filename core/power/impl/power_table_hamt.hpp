/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_POWER_POWER_TABLE_HAMT_HPP
#define CPP_FILECOIN_POWER_POWER_TABLE_HAMT_HPP

#include "power/power_table.hpp"
#include "storage/hamt/hamt.hpp"

namespace fc::power {

  using storage::hamt::Hamt;

  /**
   * @class PowerTableHamt - HAMT-based implementation of PowerTable
   */
  class PowerTableHamt : public PowerTable {
   public:
    /**
     * Construct HAMT-based power table
     * @param store - ipfs datastore
     * @param root - hamt root cid
     */
    explicit PowerTableHamt(Hamt hamt);

    /** @copydoc PowerTable::getMinerPower() */
    outcome::result<Power> getMinerPower(const Address &address) const override;

    /** @copydoc PowerTable::getMinerPower() */
    outcome::result<void> setMinerPower(const Address &address,
                                        Power power_amount) override;

    /** @copydoc PowerTable::getMinerPower() */
    outcome::result<void> removeMiner(const Address &address) override;

    /** @copydoc PowerTable::getMinerPower() */
    fc::outcome::result<size_t> getSize() const override;

    /** @copydoc PowerTable::getMinerPower() */
    fc::outcome::result<Power> getMaxPower() const override;

    /** @copydoc PowerTable::getMinerPower() */
    outcome::result<std::vector<Address>> getMiners() const override;

   private:
    /**
     * TODO (a.chernyshov) HAMT getter is not constant due to it uses cache.
     * It is a common example to use mutable field in HAMT.
     * In order to save semantic constant regarless of syntax constant
     * correctness the power_table_ field is marked as mutable.
     *
     * Remove mutable keyword after HAMT getter have const qualifier.
     */
    mutable Hamt power_table_;
  };

}  // namespace fc::power

#endif  // CPP_FILECOIN_POWER_POWER_TABLE_HAMT_HPP
