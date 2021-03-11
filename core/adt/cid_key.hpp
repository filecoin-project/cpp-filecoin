/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/cid/cid.hpp"

namespace fc::adt {
  struct CidKeyer {
    using Key = CID;

    static std::string encode(const Key &key);

    static outcome::result<Key> decode(const std::string &key);
  };
}  // namespace fc::adt
