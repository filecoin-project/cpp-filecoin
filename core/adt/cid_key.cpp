/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/cid_key.hpp"
#include "common/span.hpp"

namespace fc::adt {
  Bytes CidKeyer::encode(const Key &key) {
    return key.toBytes().value();
  }

  outcome::result<CidKeyer::Key> CidKeyer::decode(BytesIn key) {
    return CID::fromBytes(key);
  }
}  // namespace fc::adt
