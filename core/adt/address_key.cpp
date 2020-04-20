/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/address_key.hpp"
#include "common/span.hpp"

#include "primitives/address/address_codec.hpp"

namespace fc::adt {
  std::string AddressKeyer::encode(const Key &key) {
    auto bytes = primitives::address::encode(key);
    return {bytes.begin(), bytes.end()};
  }

  outcome::result<AddressKeyer::Key> AddressKeyer::decode(
      const std::string &key) {
    return primitives::address::decode(common::span::cbytes(key));
  }
}  // namespace fc::adt
