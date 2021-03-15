/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adt/cid_key.hpp"
#include "common/span.hpp"

namespace fc::adt {
  std::string CidKeyer::encode(const Key &key) {
    OUTCOME_EXCEPT(bytes, key.toBytes());
    return {bytes.begin(), bytes.end()};
  }

  outcome::result<CidKeyer::Key> CidKeyer::decode(const std::string &key) {
    return CID::fromBytes(common::span::cbytes(key));
  }
}  // namespace fc::adt
