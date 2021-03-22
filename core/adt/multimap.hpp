/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MULTIMAP_HPP
#define CPP_FILECOIN_MULTIMAP_HPP

#include "adt/array.hpp"
#include "adt/map.hpp"

namespace fc::adt {
  struct Multimap {
    template <typename Value, typename Keyer, size_t bit_width, bool v3>
    static outcome::result<void> append(
        Map<Array<Value>, Keyer, bit_width, v3> &map,
        const typename Keyer::Key &key,
        const Value &value) {
      OUTCOME_TRY(array, map.tryGet(key));
      if (!array) {
        array = Array<Value>{map.hamt.ipld};
      }
      OUTCOME_TRY(array->append(value));
      return map.set(key, *array);
    }

    template <typename Value, typename Keyer, size_t bit_width, bool v3>
    static outcome::result<void> visit(
        Map<Array<Value>, Keyer, bit_width, v3> &map,
        const typename Keyer::Key &key,
        const std::function<outcome::result<void>(const Value &)> &visitor) {
      OUTCOME_TRY(array, map.tryGet(key));
      if (array) {
        OUTCOME_TRY(
            array->visit([&](auto, auto &value) { return visitor(value); }));
      }
      return outcome::success();
    }

    template <typename Value, typename Keyer, size_t bit_width, bool v3>
    static outcome::result<std::vector<Value>> values(
        Map<Array<Value>, Keyer, bit_width, v3> &map,
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

#endif  // CPP_FILECOIN_MULTIMAP_HPP
