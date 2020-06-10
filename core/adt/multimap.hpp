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
    template <typename Value, typename Keyer, size_t bit_width>
    static outcome::result<void> append(
        Map<Array<Value>, Keyer, bit_width> &map,
        const typename Keyer::Key &key,
        const Value &value) {
      OUTCOME_TRY(array, map.tryGet(key));
      if (!array) {
        array = Array<Value>{map.hamt.ipld};
      }
      OUTCOME_TRY(array->append(value));
      return map.set(key, *array);
    }

    template <typename Value, typename Keyer, size_t bit_width>
    static outcome::result<void> visit(
        Map<Array<Value>, Keyer, bit_width> &map,
        const typename Keyer::Key &key,
        const std::function<outcome::result<void>(const Value &)> &visitor) {
      OUTCOME_TRY(array, map.tryGet(key));
      if (array) {
        OUTCOME_TRY(
            array->visit([&](auto, auto &value) { return visitor(value); }));
      }
      return outcome::success();
    }
  };
}  // namespace fc::adt

#endif  // CPP_FILECOIN_MULTIMAP_HPP
