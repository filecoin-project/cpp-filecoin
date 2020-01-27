/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP

#include "common/buffer.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::primitives::tipset {
  /**
   * @struct TipsetKey implemented according to
   * https://github.com/filecoin-project/lotus/blob/9f3b5bfb6e2c75b77ce41da42aa0a7ba89b5c411/chain/types/tipset_key.go#L27
   */
  struct TipsetKey {
    std::vector<CID> cids;

    /**
     * @brief makes human-readable representation
     * it is not the same as lotus implementation for now
     */
    std::string toPrettyString() const;

    /**
     * @brief encodes tipsetkey to a vector suitable for usage as key
     */
    outcome::result<std::vector<uint8_t>> toBytes() const;
  };

  /**
   * @brief compares 2 tipsets
   * @param lhs first tipset
   * @param rhs second tipset
   * @return true if equal, false otherwise
   */
  inline bool operator==(const TipsetKey &lhs, const TipsetKey &rhs) {
    return lhs.cids == rhs.cids;
  }

  inline bool operator!=(const TipsetKey &lhs, const TipsetKey &rhs) {
    return !(lhs.cids == rhs.cids);
  }

}  // namespace fc::primitives::tipset

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
