/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "adt/map.hpp"

namespace fc::adt {
  struct Multimap {
    template <typename Value, typename Keyer, size_t hamt_bits, size_t amt_bits>
    static outcome::result<void> append(
        Map<Array<Value, amt_bits>, Keyer, hamt_bits> &map,
        const typename Keyer::Key &key,
        const Value &value) {
      OUTCOME_TRY(array, map.tryGet(key));
      if (!array) {
        array = Array<Value, amt_bits>{map.hamt.ipld};
      }
      OUTCOME_TRY(array->append(value));
      return map.set(key, *array);
    }

    template <typename Value, typename Keyer, size_t hamt_bits, size_t amt_bits>
    static outcome::result<void> visit(
        Map<Array<Value, amt_bits>, Keyer, hamt_bits> &map,
        const typename Keyer::Key &key,
        const std::function<outcome::result<void>(const Value &)> &visitor) {
      OUTCOME_TRY(array, map.tryGet(key));
      if (array) {
        OUTCOME_TRY(
            array->visit([&](auto, auto &value) { return visitor(value); }));
      }
      return outcome::success();
    }

    template <typename Value, typename Keyer, size_t hamt_bits, size_t amt_bits>
    static outcome::result<std::vector<Value>> values(
        Map<Array<Value, amt_bits>, Keyer, hamt_bits> &map,
        const typename Keyer::Key &key) {
      OUTCOME_TRY(array, map.tryGet(key));
      std::vector<Value> values;
      if (array) {
        OUTCOME_TRYA(values, array->values());
      }
      return values;
    }
  };
}  // namespace fc::adt
