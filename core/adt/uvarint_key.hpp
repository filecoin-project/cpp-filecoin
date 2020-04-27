/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ADT_UVARINT_KEY_HPP
#define CPP_FILECOIN_ADT_UVARINT_KEY_HPP

#include <string>

#include "common/outcome.hpp"

namespace fc::adt {
  enum class UvarintKeyError { DECODE_ERROR = 1 };

  struct UvarintKeyer {
    using Key = uint64_t;

    static std::string encode(Key key);

    static outcome::result<Key> decode(const std::string &key);
  };

  struct VarintKeyer {
    using Key = int64_t;

    static std::string encode(Key key);

    static outcome::result<Key> decode(const std::string &key);
  };
}  // namespace fc::adt

OUTCOME_HPP_DECLARE_ERROR(fc::adt, UvarintKeyError);

#endif  // CPP_FILECOIN_ADT_UVARINT_KEY_HPP
