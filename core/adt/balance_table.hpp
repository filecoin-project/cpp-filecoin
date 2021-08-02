/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "primitives/types.hpp"

namespace fc::adt {
  using primitives::TokenAmount;

  enum class BalanceTableError { kInsufficientFunds = 1 };

  struct BalanceTable : public Map<TokenAmount, AddressKeyer, 6> {
    using Map::Map;

    outcome::result<void> add(const Key &key, TokenAmount amount);

    outcome::result<void> addCreate(const Key &key, TokenAmount amount);

    outcome::result<TokenAmount> subtractWithMin(const Key &key,
                                                 TokenAmount amount,
                                                 TokenAmount min);

    outcome::result<void> subtract(const Key &key, TokenAmount amount);
  };
}  // namespace fc::adt

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<adt::BalanceTable> {
    template <typename Visitor>
    static void call(adt::Map<adt::TokenAmount, adt::AddressKeyer, 6> &map,
                     const Visitor &visit) {
      visit(map);
    }
  };
}  // namespace fc::cbor_blake

OUTCOME_HPP_DECLARE_ERROR(fc::adt, BalanceTableError);
