/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/cid/cid.hpp"

namespace fc::adt {
  struct CidKeyer {
    using Key = CID;

    static Bytes encode(const Key &key);

    static outcome::result<Key> decode(BytesIn key);
  };
}  // namespace fc::adt
