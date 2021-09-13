/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/uvarint_key.hpp"
#include "codec/uvarint.hpp"
#include "common/span.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::adt, UvarintKeyError, e) {
  using E = fc::adt::UvarintKeyError;
  switch (e) {
    case E::kDecodeError:
      return "Decode error";
  }
}

namespace fc::adt {
  Bytes UvarintKeyer::encode(Key value) {
    return copy(codec::uvarint::VarintEncoder{value}.bytes());
  }

  outcome::result<UvarintKeyer::Key> UvarintKeyer::decode(BytesIn key) {
    Key result = 0;
    if (codec::uvarint::read(result, key) && key.empty()) {
      return result;
    }
    return UvarintKeyError::kDecodeError;
  }

  Bytes VarintKeyer::encode(Key value) {
    value <<= 1;
    return UvarintKeyer::encode(value < 0 ? ~value : value);
  }

  outcome::result<VarintKeyer::Key> VarintKeyer::decode(BytesIn key) {
    OUTCOME_TRY(value2, UvarintKeyer::decode(key));
    Key value = static_cast<Key>(value2) >> 1;
    return (value2 & 1) != 0 ? ~value : value;
  }
}  // namespace fc::adt
