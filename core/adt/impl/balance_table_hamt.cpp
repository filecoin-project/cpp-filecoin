/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/balance_table_hamt.hpp"
#include "primitives/address/address_codec.hpp"

using fc::adt::BalanceTableHamt;
using fc::adt::TokenAmount;
using fc::primitives::address::encodeToByteString;

BalanceTableHamt::BalanceTableHamt(std::shared_ptr<IpfsDatastore> datastore,
                                   const CID &new_root)
    : root{new_root}, hamt_{datastore, new_root} {}

fc::outcome::result<TokenAmount> BalanceTableHamt::get(
    const Address &key) const {
  return hamt_.getCbor<TokenAmount>(encodeToByteString(key));
}

fc::outcome::result<bool> BalanceTableHamt::has(const Address &key) const {
  return hamt_.contains(encodeToByteString(key));
}

fc::outcome::result<void> BalanceTableHamt::set(const Address &key,
                                                const TokenAmount &balance) {
  OUTCOME_TRY(hamt_.setCbor(encodeToByteString(key), balance));
  OUTCOME_TRY(new_root, hamt_.flush());
  root = new_root;
  return fc::outcome::success();
}

fc::outcome::result<void> BalanceTableHamt::add(const Address &key,
                                                const TokenAmount &amount) {
  OUTCOME_TRY(balance, hamt_.getCbor<TokenAmount>(encodeToByteString(key)));
  OUTCOME_TRY(hamt_.setCbor(encodeToByteString(key), balance + amount));
  OUTCOME_TRY(new_root, hamt_.flush());
  root = new_root;
  return fc::outcome::success();
}

fc::outcome::result<TokenAmount> BalanceTableHamt::subtractWithMinimum(
    const Address &key, const TokenAmount &amount, const TokenAmount &floor) {
  OUTCOME_TRY(balance, hamt_.getCbor<TokenAmount>(encodeToByteString(key)));
  TokenAmount available = balance < floor ? TokenAmount{0} : balance - floor;
  TokenAmount sub = std::min(available, amount);
  OUTCOME_TRY(hamt_.setCbor(encodeToByteString(key), balance - sub));
  OUTCOME_TRY(new_root, hamt_.flush());
  root = new_root;
  return sub;
}

fc::outcome::result<TokenAmount> BalanceTableHamt::remove(const Address &key) {
  OUTCOME_TRY(old_balance, hamt_.getCbor<TokenAmount>(encodeToByteString(key)));
  OUTCOME_TRY(hamt_.remove(encodeToByteString(key)));
  OUTCOME_TRY(new_root, hamt_.flush());
  root = new_root;
  return std::move(old_balance);
}
