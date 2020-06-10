/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ADT_SET_HPP
#define CPP_FILECOIN_ADT_SET_HPP

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

  template <typename Keyer>
  struct Set : Map<SetValue, Keyer> {};
}  // namespace fc::adt

namespace fc {
  template <typename Keyer>
  struct Ipld::Visit<adt::Set<Keyer>> {
    template <typename Visitor>
    static void call(adt::Map<adt::SetValue, Keyer> &map,
                     const Visitor &visit) {
      visit(map);
    }
  };
}  // namespace fc

#endif  // CPP_FILECOIN_ADT_SET_HPP
