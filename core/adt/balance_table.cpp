/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/balance_table.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::adt, BalanceTableError, e) {
  using E = fc::adt::BalanceTableError;
  switch (e) {
    case E::INSUFFICIENT_FUNDS:
      return "Insufficient funds";
  }
}

namespace fc::adt {
  outcome::result<void> BalanceTable::add(const Key &key, TokenAmount amount) {
    OUTCOME_TRY(value, get(key));
    value += amount;
    OUTCOME_TRY(set(key, value));
    return outcome::success();
  }

  outcome::result<void> BalanceTable::addCreate(const Key &key, TokenAmount amount) {
    OUTCOME_TRY(value, tryGet(key));
    if (value) {
      amount += *value;
    }
    return set(key, amount);
  }

  outcome::result<TokenAmount> BalanceTable::subtractWithMin(const Key &key,
                                                             TokenAmount amount,
                                                             TokenAmount min) {
    OUTCOME_TRY(value, get(key));
    TokenAmount subtracted =
        std::min(amount, std::max(TokenAmount{value - min}, TokenAmount{0}));
    OUTCOME_TRY(set(key, value - subtracted));
    return subtracted;
  }

  outcome::result<void> BalanceTable::subtract(const Key &key,
                                               TokenAmount amount) {
    OUTCOME_TRY(substracted, subtractWithMin(key, amount, 0));
    if (substracted != amount) {
      return BalanceTableError::INSUFFICIENT_FUNDS;
    }
    return outcome::success();
  }
}  // namespace fc::adt
