/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPLD_SELECTOR_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPLD_SELECTOR_HPP

#include "codec/cbor/cbor_raw.hpp"

namespace fc::storage::ipld {

  // TODO(turuslan): implement selectors
  using Selector = CborRaw;

  /**
   * Selector to walk through all nodes
   * {"R": {"l": {"none": {}}, ":>": {"a": {">": {"@": {}}}}}}
   */
  static const Selector kAllSelector{
      Buffer::fromHex("a16152a2616ca1646e6f6e65a0623a3ea16161a1613ea16140a0")
          .value()};
}  // namespace fc::storage::ipld

#endif  // CPP_FILECOIN_CORE_STORAGE_IPLD_SELECTOR_HPP
