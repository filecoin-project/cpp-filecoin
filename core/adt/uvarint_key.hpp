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

  struct UvarintKey {
    using Type = uint64_t;

    static std::string encode(Type key);

    static outcome::result<Type> decode(const std::string &key);
  };
}  // namespace fc::adt

OUTCOME_HPP_DECLARE_ERROR(fc::adt, UvarintKeyError);

#endif  // CPP_FILECOIN_ADT_UVARINT_KEY_HPP
