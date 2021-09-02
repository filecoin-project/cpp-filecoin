/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/address_key.hpp"
#include "common/span.hpp"

#include "primitives/address/address_codec.hpp"

namespace fc::adt {
  Bytes AddressKeyer::encode(const Key &key) {
    return Bytes{primitives::address::encode(key)};
  }

  outcome::result<AddressKeyer::Key> AddressKeyer::decode(BytesIn key) {
    return primitives::address::decode(key);
  }
}  // namespace fc::adt
