/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPLD_SELECTOR_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPLD_SELECTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "common/hexutil.hpp"

namespace fc::storage::ipld {

  // TODO(turuslan): implement selectors
  struct Selector {
    common::Buffer raw;
  };
  CBOR_ENCODE(Selector, selector) {
    return s << s.wrap(selector.raw, 1);
  }
  CBOR_DECODE(Selector, selector) {
    selector.raw = common::Buffer{s.raw()};
    return s;
  }

  /**
   * Selector to walk through all nodes
   * {"R": {"l": {"none": {}}, ":>": {"a": {">": {"@": {}}}}}}
   */
  static const Selector kAllSelector{
      .raw = common::Buffer{
          common::unhex(
              std::string(
                  "a16152a2616ca1646e6f6e65a0623a3ea16161a1613ea16140a0"))
              .value()}};

}  // namespace fc::storage::ipld

#endif  // CPP_FILECOIN_CORE_STORAGE_IPLD_SELECTOR_HPP
