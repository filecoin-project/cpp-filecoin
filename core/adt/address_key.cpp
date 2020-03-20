/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/address_key.hpp"

#include "primitives/address/address_codec.hpp"

namespace fc::adt {
  std::string AddressKey::encode(const Type &key) {
    auto bytes = primitives::address::encode(key);
    return {bytes.begin(), bytes.end()};
  }

  outcome::result<AddressKey::Type> AddressKey::decode(const std::string &key) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return primitives::address::decode(gsl::make_span(
        reinterpret_cast<const uint8_t *>(key.data()), key.size()));
  }
}  // namespace fc::adt
