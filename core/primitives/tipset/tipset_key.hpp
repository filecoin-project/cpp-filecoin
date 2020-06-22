/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP

#include "primitives/cid/cid.hpp"

namespace fc::primitives::tipset {

  enum class HashError { HASH_INITIALIZE_ERROR = 1 };

  using TipsetHash = std::vector<uint8_t>;

  /// Returns blake2b-256 from concatenated cids bytes
  outcome::result<TipsetHash> tipsetHash(const std::vector<CID> &cids);

  /// Returns hex string repr of hash
  std::string tipsetHashToString(const TipsetHash& hash);

  /// TipsetKey: block cids + hash
  class TipsetKey {
   public:
    /// Creates TipsetKey and calculates hash
    static outcome::result<TipsetKey> create(std::vector<CID> cids);

    TipsetKey();
    TipsetKey(const TipsetKey &) = default;
    TipsetKey(TipsetKey &&) = default;
    TipsetKey &operator=(const TipsetKey &other) = default;
    TipsetKey &operator=(TipsetKey &&other) = default;

    bool operator==(const TipsetKey &rhs) const;
    bool operator!=(const TipsetKey &rhs) const;

    const std::vector<CID>& cids() const;

    const TipsetHash& hash() const;

    /// Returns hash string representaion
    std::string toString() const;

    /// Returns string representaion based on contained CIDs
    std::string toPrettyString() const;

   private:
    TipsetKey(std::vector<CID> c, TipsetHash h);

    std::vector<CID> cids_;
    TipsetHash hash_;
  };

}  // namespace fc::primitives::tipset

namespace std {
  template <>
  struct hash<fc::primitives::tipset::TipsetKey> {
    size_t operator()(const fc::primitives::tipset::TipsetKey &x) const;
  };
}  // namespace std

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::tipset, HashError);

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
