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
        h.resize(crypto::blake2b::BLAKE2B256_HASH_LENGTH, 0xFF);
        return h;
      }();
      return ones;
    }

  }  // namespace

  TipsetHash TipsetKey::hash(const std::vector<CID> &cids) {
    if (cids.empty()) {
      return emptyTipsetHash();
    }

    crypto::blake2b::Ctx ctx(crypto::blake2b::BLAKE2B256_HASH_LENGTH);

    size_t sz = cids.size();
    auto indices = (size_t *)alloca(sizeof(size_t) * sz);
    auto ptr = indices;
    for (size_t i = 0; i < sz; ++i, ++ptr) {
      *ptr = i;
    }

    if (sz > 1) {
      std::sort(indices, indices + sz, [&cids](auto x, auto y) {
        return cids[x] < cids[y];
      });
    }

    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < sz; ++i) {
      // TODO: may manually hash cid parts to avoid exceptions
      bytes = cids[indices[i]].toBytes().value();
      ctx.update(bytes);
    }

    TipsetHash hash;
    hash.resize(crypto::blake2b::BLAKE2B256_HASH_LENGTH);

    ctx.final(hash);
    return hash;
  }

  TipsetKey::TipsetKey() : hash_(emptyTipsetHash()) {}

  TipsetKey::TipsetKey(std::vector<CID> c)
      : hash_{hash(c)}, cids_{std::move(c)} {}

  bool TipsetKey::operator==(const TipsetKey &rhs) const {
    return hash_ == rhs.hash_;
  }

  bool TipsetKey::operator!=(const TipsetKey &rhs) const {
    return hash_ != rhs.hash_;
  }

  bool TipsetKey::operator<(const TipsetKey &rhs) const {
    return hash_ < rhs.hash_;
  }

  const std::vector<CID> &TipsetKey::cids() const {
    return cids_;
  }

  const TipsetHash &TipsetKey::hash() const {
    return hash_;
  }

  std::string TipsetKey::toString() const {
    return common::hex_lower(hash_);
  }

  std::string TipsetKey::toPrettyString() const {
    return fmt::format("{{{}}}", fmt::join(cids_, ","));
  }
}  // namespace fc::primitives::tipset

namespace std {
  size_t hash<fc::TipsetKey>::operator()(
      const fc::TipsetKey &x) const {
    size_t h;
    memcpy(&h, x.hash().data(), sizeof(size_t));
    return h;
  }
}  // namespace std
