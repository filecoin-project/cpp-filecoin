/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/cid/cid.hpp"

namespace fc::primitives::tipset {
  using TipsetHash = Hash256;

  /// TipsetKey: block cids + hash
  class TipsetKey {
   public:
    static TipsetHash hash(CbCidsIn cids);
    static boost::optional<TipsetKey> make(gsl::span<const CID> cids);

    TipsetKey();
    // TODO (a.chernyshov) make constructors explicit (FIL-415)
    // NOLINTNEXTLINE(google-explicit-constructor)
    TipsetKey(std::vector<CbCid> c);
    TipsetKey(TipsetHash h, std::vector<CbCid> c);

    bool operator==(const TipsetKey &rhs) const;
    bool operator<(const TipsetKey &rhs) const;

    const std::vector<CbCid> &cids() const;

    const TipsetHash &hash() const;

    /// Returns hash string representaion
    std::string toString() const;

    std::string cidsStr(std::string_view sep = ",") const;

   private:
    TipsetHash hash_;
    std::vector<CbCid> cids_;
  };
  FC_OPERATOR_NOT_EQUAL(TipsetKey)
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
