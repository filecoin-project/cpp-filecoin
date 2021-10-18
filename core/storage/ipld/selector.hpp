/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_raw.hpp"

namespace fc::storage::ipld {

  // TODO(turuslan): implement selectors
  using Selector = CborRaw;

  /**
   * Selector to walk through all nodes
   * {"R": {"l": {"none": {}}, ":>": {"a": {">": {"@": {}}}}}}
   */
  static const Selector kAllSelector{
      fromHex("a16152a2616ca1646e6f6e65a0623a3ea16161a1613ea16140a0")
          .value()};
}  // namespace fc::storage::ipld
