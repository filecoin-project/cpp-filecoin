/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/container_hash/hash.hpp>

#include "primitives/tipset/tipset_key.hpp"

namespace fc::primitives::tipset {
  TipsetKey::TipsetKey(std::vector<CID> cids)
      : cids{cids}, hash{boost::hash_range(cids.begin(), cids.end())} {}

  TipsetKey::TipsetKey(std::initializer_list<CID> cids)
      : TipsetKey{std::vector<CID>{cids}} {}
}  // namespace fc::primitives::tipset
