/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/balance_table_hamt.hpp"
#include "adt/address_key.hpp"

using fc::adt::BalanceTableHamt;
using fc::adt::TokenAmount;

BalanceTableHamt::BalanceTableHamt(std::shared_ptr<IpfsDatastore> datastore,
                                   const CID &new_root)
    : root{new_root}, hamt_{datastore, new_root} {}

fc::outcome::result<TokenAmount> BalanceTableHamt::get(
    const Address &key) const {
  return hamt_.getCbor<TokenAmount>(AddressKeyer::encode(key));
}

fc::outcome::result<bool> BalanceTableHamt::has(const Address &key) const {
  return hamt_.contains(AddressKeyer::encode(key));
}

fc::outcome::result<void> BalanceTableHamt::set(const Address &key,
                                                const TokenAmount &balance) {
  OUTCOME_TRY(hamt_.setCbor(AddressKeyer::encode(key), balance));
  OUTCOME_TRY(new_root, hamt_.flush());
  root = new_root;
  return fc::outcome::success();
}

fc::outcome::result<void> BalanceTableHamt::add(const Address &key,
                                                const TokenAmount &amount) {
  OUTCOME_TRY(balance, hamt_.getCbor<TokenAmount>(AddressKeyer::encode(key)));
  OUTCOME_TRY(hamt_.setCbor(AddressKeyer::encode(key), balance + amount));
  OUTCOME_TRY(new_root, hamt_.flush());
  root = new_root;
  return fc::outcome::success();
}

fc::outcome::result<TokenAmount> BalanceTableHamt::subtractWithMinimum(
    const Address &key, const TokenAmount &amount, const TokenAmount &floor) {
  OUTCOME_TRY(balance, hamt_.getCbor<TokenAmount>(AddressKeyer::encode(key)));
  TokenAmount available = balance < floor ? TokenAmount{0} : balance - floor;
  TokenAmount sub = std::min(available, amount);
  OUTCOME_TRY(hamt_.setCbor(AddressKeyer::encode(key), balance - sub));
  OUTCOME_TRY(new_root, hamt_.flush());
  root = new_root;
  return sub;
}

fc::outcome::result<TokenAmount> BalanceTableHamt::remove(const Address &key) {
  OUTCOME_TRY(old_balance,
              hamt_.getCbor<TokenAmount>(AddressKeyer::encode(key)));
  OUTCOME_TRY(hamt_.remove(AddressKeyer::encode(key)));
  OUTCOME_TRY(new_root, hamt_.flush());
  root = new_root;
  return std::move(old_balance);
}
