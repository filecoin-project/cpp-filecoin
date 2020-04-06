/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/uvarint_key.hpp"

#include <libp2p/multi/uvarint.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(fc::adt, UvarintKeyError, e) {
  using E = fc::adt::UvarintKeyError;
  switch (e) {
    case E::DECODE_ERROR:
      return "Decode error";
  }
}

namespace fc::adt {
  using libp2p::multi::UVarint;

  std::string UvarintKeyer::encode(Key value) {
    UVarint uvarint{value};
    auto encoded = uvarint.toBytes();
    return std::string(encoded.begin(), encoded.end());
  }

  outcome::result<UvarintKeyer::Key> UvarintKeyer::decode(
      const std::string &key) {
    auto maybe = UVarint::create(gsl::make_span(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t *>(key.data()),
        key.size()));
    if (!maybe) {
      return UvarintKeyError::DECODE_ERROR;
    }
    return maybe->toUInt64();
  }
}  // namespace fc::adt
