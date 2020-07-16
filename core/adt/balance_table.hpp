/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ADT_BALANCE_TABLE_HPP
#define CPP_FILECOIN_ADT_BALANCE_TABLE_HPP

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "primitives/types.hpp"

namespace fc::adt {
  using primitives::TokenAmount;

  enum class BalanceTableError { kInsufficientFunds = 1 };

  struct BalanceTable : public Map<TokenAmount, AddressKeyer> {
    using Map::Map;

    outcome::result<void> add(const Key &key, TokenAmount amount);

    outcome::result<void> addCreate(const Key &key, TokenAmount amount);

    outcome::result<TokenAmount> subtractWithMin(const Key &key,
                                                 TokenAmount amount,
                                                 TokenAmount min);

    outcome::result<void> subtract(const Key &key, TokenAmount amount);
  };
}  // namespace fc::adt

namespace fc {
  template <>
  struct Ipld::Visit<adt::BalanceTable> {
    template <typename Visitor>
    static void call(adt::Map<adt::TokenAmount, adt::AddressKeyer> &map,
                     const Visitor &visit) {
      visit(map);
    }
  };
}  // namespace fc

OUTCOME_HPP_DECLARE_ERROR(fc::adt, BalanceTableError);

#endif  // CPP_FILECOIN_ADT_BALANCE_TABLE_HPP
