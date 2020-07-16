/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/uvarint_key.hpp"
#include "common/span.hpp"

#include <libp2p/multi/uvarint.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(fc::adt, UvarintKeyError, e) {
  using E = fc::adt::UvarintKeyError;
  switch (e) {
    case E::kDecodeError:
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
    auto maybe = UVarint::create(common::span::cbytes(key));
    if (!maybe) {
      return UvarintKeyError::kDecodeError;
    }
    return maybe->toUInt64();
  }

  std::string VarintKeyer::encode(Key value) {
    value <<= 1;
    return UvarintKeyer::encode(value < 0 ? ~value : value);
  }

  outcome::result<VarintKeyer::Key> VarintKeyer::decode(
      const std::string &key) {
    OUTCOME_TRY(value2, UvarintKeyer::decode(key));
    int64_t value = value2 >> 1;
    return value & 1 ? ~value : value;
  }
}  // namespace fc::adt
