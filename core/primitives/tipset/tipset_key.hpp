/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP

#include "primitives/cid/cid.hpp"

namespace fc::primitives::tipset {
  /**
   * @struct TipsetKey implemented according to
   * https://github.com/filecoin-project/lotus/blob/9f3b5bfb6e2c75b77ce41da42aa0a7ba89b5c411/chain/types/tipset_key.go#L27
   */
  struct TipsetKey {
    std::vector<CID> cids;
    std::size_t hash{};

    TipsetKey() = default;
    TipsetKey(std::vector<CID> cids);
    TipsetKey(std::initializer_list<CID> cids);
  };

  /**
   * @brief checks if tipsets are equal
   * @param lhs first tipset
   * @param rhs second tipset
   * @return true if equal, false otherwise
   */
  inline bool operator==(const TipsetKey &lhs, const TipsetKey &rhs) {
    return lhs.cids == rhs.cids;
  }

  /**
   * @brief checks if tipsets are not equal
   * @param lhs first tipset
   * @param rhs second tipset
   * @return true if not equal, false otherwise
   */
  inline bool operator!=(const TipsetKey &lhs, const TipsetKey &rhs) {
    return !(lhs == rhs);
  }

}  // namespace fc::primitives::tipset

namespace std {
  template <>
  struct hash<fc::primitives::tipset::TipsetKey> {
    size_t operator()(const fc::primitives::tipset::TipsetKey &x) const {
      return x.hash;
    }
  };
}  // namespace std

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
