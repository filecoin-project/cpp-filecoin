/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/bytes.hpp"
#include "primitives/address/address.hpp"

namespace fc::adt {
  struct AddressKeyer {
    using Key = primitives::address::Address;

    static Bytes encode(const Key &key);

    static outcome::result<Key> decode(BytesIn key);
  };
}  // namespace fc::adt
