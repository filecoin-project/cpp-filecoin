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
  class TipsetKey {
    std::vector<CID> cids_;
    std::size_t hash_;

    TipsetKey(std::vector<CID> cids, std::size_t hash)
        : cids_{std::move(cids)}, hash_{hash} {}

   public:
    /** @brief creates TipsetKey and calculates hash */
    static outcome::result<TipsetKey> create(std::vector<CID> cids);

    /**
     * @brief makes human-readable representation
     * it is not the same as lotus implementation for now
     */
    std::string toPrettyString() const;

    /**
     * @brief encodes tipsetkey to a vector suitable for usage as key
     */
    outcome::result<std::vector<uint8_t>> toBytes() const;

    /** @brief provides readonly access to vector of cids */
    const auto &getCids() const {
      return cids_;
    }

    /** @brief provides readonly access to hash value */
    const auto &getHash() const {
      return hash_;
    }
  };

  /**
   * @brief checks if tipsets are equal
   * @param lhs first tipset
   * @param rhs second tipset
   * @return true if equal, false otherwise
   */
  inline bool operator==(const TipsetKey &lhs, const TipsetKey &rhs) {
    return lhs.getCids() == rhs.getCids();
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
      return x.getHash();
    }
  };
}  // namespace std

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
