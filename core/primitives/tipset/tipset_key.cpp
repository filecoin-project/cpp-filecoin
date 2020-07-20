/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset_key.hpp"

#include "crypto/blake2/blake2b.h"
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

  outcome::result<TipsetHash> tipsetHash(const std::vector<CID> &cids) {
    if (cids.empty()) {
      return emptyTipsetHash();
    }

    blake2b_ctx ctx;

    if (blake2b_init(&ctx, crypto::blake2b::BLAKE2B256_HASH_LENGTH, nullptr, 0)
        != 0) {
      return HashError::HASH_INITIALIZE_ERROR;
    }

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
      OUTCOME_TRYA(bytes, cids[indices[i]].toBytes());
      blake2b_update(&ctx, bytes.data(), bytes.size());
    }

    TipsetHash hash;
    hash.resize(crypto::blake2b::BLAKE2B256_HASH_LENGTH);

    blake2b_final(&ctx, hash.data());
    return hash;
  }

  std::string tipsetHashToString(const TipsetHash &hash) {
    return common::hex_lower(hash);
  }

  TipsetKey::TipsetKey() : hash_(emptyTipsetHash()) {}

  TipsetKey::TipsetKey(std::vector<CID> c, TipsetHash h)
      : cids_(std::move(c)), hash_(std::move(h)) {}

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
    return tipsetHashToString(hash_);
  }

  std::string TipsetKey::toPrettyString() const {
    if (cids_.empty()) {
      return "{}";
    }

    std::string result = "{";
    auto size = cids_.size();
    for (size_t i = 0; i < size; ++i) {
      result += cids_[i].toPrettyString("");
      if (i != size - 1) {
        result += ", ";
      }
    }
    result += "}";

    return result;
  }

  outcome::result<TipsetKey> TipsetKey::create(std::vector<CID> cids) {
    OUTCOME_TRY(hash, tipsetHash(cids));
    return TipsetKey{std::move(cids), std::move(hash)};
  }

  TipsetKey TipsetKey::create(std::vector<CID> cids, TipsetHash hash) {
    assert(tipsetHash(cids).value() == hash);
    return TipsetKey{std::move(cids), std::move(hash)};
  }

}  // namespace fc::primitives::tipset

namespace std {
  size_t hash<fc::primitives::tipset::TipsetKey>::operator()(
      const fc::primitives::tipset::TipsetKey &x) const {
    size_t h;
    memcpy(&h, x.hash().data(), sizeof(size_t));
    return h;
  }
}  // namespace std

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::tipset, HashError, e) {
  using fc::primitives::tipset::HashError;

  switch (e) {
    case HashError::HASH_INITIALIZE_ERROR:
      return "Hash engine ctx initialization error";
    default:
      break;
  }

  return "Unknown error";
}
