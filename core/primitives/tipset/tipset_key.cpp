/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/container_hash/hash.hpp>

#include "primitives/tipset/tipset_key.hpp"

#include "crypto/blake2/blake2b160.hpp"

namespace fc::primitives::tipset {

  namespace {

    TipsetHash emptyTipsetHash() {
      static const TipsetHash ones = [] {
        TipsetHash h;
        h.fill(0xFF);
        return h;
      }();
      return ones;
    }

  }  // namespace

  TipsetHash TipsetKey::hash(CbCidsIn cids) {
    if (cids.empty()) {
      return emptyTipsetHash();
    }

    crypto::blake2b::Ctx ctx(crypto::blake2b::BLAKE2B256_HASH_LENGTH);

    size_t sz = cids.size();
    std::array<uint8_t, 254> indices;
    assert((size_t)cids.size() <= indices.size());
    for (size_t i = 0; i < sz; ++i) {
      indices[i] = i;
    }
    std::sort(indices.data(),
              indices.data() + cids.size(),
              [&](auto &l, auto &r) { return cids[l] < cids[r]; });
    for (size_t i = 0; i < sz; ++i) {
      ctx.update(kCborBlakePrefix);
      ctx.update(cids[indices[i]]);
    }

    TipsetHash hash;
    ctx.final(hash);
    return hash;
  }

  boost::optional<TipsetKey> TipsetKey::make(gsl::span<const CID> cids) {
    std::vector<CbCid> hashes;
    for (auto &cid : cids) {
      if (auto hash{asBlake(cid)}) {
        hashes.push_back(*hash);
      } else {
        return boost::none;
      }
    }
    return TipsetKey{std::move(hashes)};
  }

  TipsetKey::TipsetKey() : hash_(emptyTipsetHash()) {}

  TipsetKey::TipsetKey(std::vector<CbCid> c)
      : hash_{hash(c)}, cids_{std::move(c)} {}

  TipsetKey::TipsetKey(TipsetHash h, std::vector<CbCid> c)
      : hash_{std::move(h)}, cids_{std::move(c)} {
    assert(hash_ == hash(cids_));
  }

  bool TipsetKey::operator==(const TipsetKey &rhs) const {
    return hash_ == rhs.hash_;
  }

  bool TipsetKey::operator<(const TipsetKey &rhs) const {
    return hash_ < rhs.hash_;
  }

  const std::vector<CbCid> &TipsetKey::cids() const {
    return cids_;
  }

  const TipsetHash &TipsetKey::hash() const {
    return hash_;
  }

  std::string TipsetKey::toString() const {
    return common::hex_lower(hash_);
  }

  std::string TipsetKey::cidsStr(std::string_view sep) const {
    std::string s;
    auto it{cids_.begin()};
    if (it != cids_.end()) {
      s += common::hex_lower(*it);
      ++it;
      while (it != cids_.end()) {
        s += sep;
        s += common::hex_lower(*it);
        ++it;
      }
    }
    return s;
  }
}  // namespace fc::primitives::tipset

namespace std {
  size_t hash<fc::TipsetKey>::operator()(const fc::TipsetKey &x) const {
    return *(const size_t *)x.hash().data();
  }
}  // namespace std
