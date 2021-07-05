/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/bytes.hpp"
#include "common/outcome.hpp"

namespace fc::adt {
  enum class UvarintKeyError { kDecodeError = 1 };

  struct UvarintKeyer {
    using Key = uint64_t;

    static Bytes encode(Key key);

    static outcome::result<Key> decode(BytesIn key);
  };

  struct VarintKeyer {
    using Key = int64_t;

    static Bytes encode(Key key);

    static outcome::result<Key> decode(BytesIn key);
  };
}  // namespace fc::adt

OUTCOME_HPP_DECLARE_ERROR(fc::adt, UvarintKeyError);
