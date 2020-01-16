/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP

#include "primitives/cid/cid.hpp"

#include <boost/optional.hpp>

namespace fc::primitives::block {
  struct BlockHeader;
}

namespace fc::primitives::tipset {

  /**
   * @struct Tipset
   */
  struct Tipset {
    std::vector<CID> cids;
    std::vector<boost::optional<block::BlockHeader>> blks;
  };



}  // namespace fc::primitives::tipset

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
