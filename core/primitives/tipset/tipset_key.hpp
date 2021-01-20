/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP

#include "primitives/cid/cid.hpp"

namespace fc::primitives::tipset {
  using TipsetHash = std::vector<uint8_t>;

  /// TipsetKey: block cids + hash
  class TipsetKey {
   public:
    static TipsetHash hash(const std::vector<CID> &cids);

    TipsetKey();
    TipsetKey(std::vector<CID> c);
    TipsetKey(TipsetHash h, std::vector<CID> c);

    bool operator==(const TipsetKey &rhs) const;
    bool operator!=(const TipsetKey &rhs) const;
    bool operator<(const TipsetKey &rhs) const;

    const std::vector<CID> &cids() const;

    const TipsetHash &hash() const;

    /// Returns hash string representaion
    std::string toString() const;

    /// Returns string representaion based on contained CIDs
    std::string toPrettyString() const;

   private:
    TipsetHash hash_;
    std::vector<CID> cids_;
  };

}  // namespace fc::primitives::tipset

namespace fc {
  using primitives::tipset::TipsetKey;
}  // namespace fc

namespace std {
  template <>
  struct hash<fc::TipsetKey> {
    size_t operator()(const fc::TipsetKey &x) const;
  };
}  // namespace std

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
