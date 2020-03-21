/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/address_key.hpp"

#include "primitives/address/address_codec.hpp"

namespace fc::adt {
  std::string AddressKeyer::encode(const Key &key) {
    auto bytes = primitives::address::encode(key);
    return {bytes.begin(), bytes.end()};
  }

  outcome::result<AddressKeyer::Key> AddressKeyer::decode(
      const std::string &key) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return primitives::address::decode(gsl::make_span(
        reinterpret_cast<const uint8_t *>(key.data()), key.size()));
  }
}  // namespace fc::adt
