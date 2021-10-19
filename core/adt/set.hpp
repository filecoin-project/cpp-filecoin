/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/map.hpp"

namespace fc::adt {
  struct SetValue {};
  CBOR_ENCODE(SetValue, v) {
    return s << nullptr;
  }
  CBOR_DECODE(SetValue, v) {
    s.next();
    return s;
  }

  template <typename Keyer, size_t bit_width = storage::hamt::kDefaultBitWidth>
  struct Set : Map<SetValue, Keyer, bit_width> {
    using Key = typename Keyer::Key;

    outcome::result<void> set(const Key &key) {
      return Map<SetValue, Keyer, bit_width>::set(key, {});
    }
  };
}  // namespace fc::adt

namespace fc::cbor_blake {
  template <typename Keyer, size_t bit_width>
  struct CbVisitT<adt::Set<Keyer, bit_width>> {
    template <typename Visitor>
    static void call(adt::Map<adt::SetValue, Keyer, bit_width> &map,
                     const Visitor &visit) {
      visit(map);
    }
  };
}  // namespace fc::cbor_blake
