/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ADT_BALANCE_TABLE_HAMT_HPP
#define CPP_FILECOIN_ADT_BALANCE_TABLE_HAMT_HPP

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/hamt/hamt.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::adt {

  using primitives::BigInt;
  using primitives::address::Address;
  using storage::hamt::Hamt;
  using storage::ipfs::IpfsDatastore;
  using TokenAmount = BigInt;

  /**
   * Stores miner balances
   */
  class BalanceTableHamt {
   public:
    /**
     * Constructor
     * @param datastore - internal store
     * @param new_root - HAMT root CID
     */
    explicit BalanceTableHamt(std::shared_ptr<IpfsDatastore> datastore,
                              const CID &new_root);

    /**
     * Get balance
     * @param key - address
     * @return balance or error
     */
    outcome::result<TokenAmount> get(const Address &key) const;

    /**
     * Checks if address is present in table
     * @param key address
     * @return true or false
     */
    outcome::result<bool> has(const Address &key) const;

    /**
     * Store balance
     * @param key address
     * @param balance token amount
     * @return error in case of failure
     */
    outcome::result<void> set(const Address &key, const TokenAmount &balance);

    /**
     * Add amount to balance
     * @param key address
     * @param amount to add
     * @return error in case of failure
     */
    outcome::result<void> add(const Address &key, const TokenAmount &amount);

    /**
     * Subtracts up to the specified amount from a balance, without reducing the
     * balance below some minimum
     * @param key address
     * @param amount to subtract
     * @param floor - minimum
     * @return the amount subtracted
     */
    outcome::result<TokenAmount> subtractWithMinimum(const Address &key,
                                                     const TokenAmount &amount,
                                                     const TokenAmount &floor);

    /**
     * Removes from table
     * @param key addrss
     * @return error in case of failure
     */
    outcome::result<void> remove(const Address &key);

    /**
     * HAMT CID of root
     */
    CID root;

   private:
    mutable Hamt hamt_;
  };

}  // namespace fc::adt

#endif  // CPP_FILECOIN_ADT_BALANCE_TABLE_HAMT_HPP
