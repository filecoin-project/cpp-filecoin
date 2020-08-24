/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/tipset_cache_impl.hpp"

#include "primitives/tipset/tipset.hpp"

namespace fc::mining {

  int64_t normalModulo(int64_t n, int64_t m) {
    return ((n % m) + m) % m;
  }

  TipsetCacheImpl::TipsetCacheImpl(uint64_t capability,
                                   GetTipsetFunction get_function)
      : get_function_(std::move(get_function)) {
    cache_.reserve(capability);
    start_ = 0;
    len_ = 0;
    cache_[start_] = boost::none;
  }

  outcome::result<void> TipsetCacheImpl::add(const Tipset &tipset) {
    if (len_ > 0) {
      if (cache_[start_]->height >= tipset.height) {
        return TipsetCacheError::kSmallerHeight;
      }
    }

    auto next_height = tipset.height;
    if (len_ > 0) {
      next_height = cache_[start_]->height + 1;
    }

    while (next_height != tipset.height) {
      start_ = normalModulo(start_ + 1, cache_.size());
      cache_[start_] = boost::none;
      if (len_ < cache_.size()) {
        len_++;
      }
      next_height++;
    }

    start_ = normalModulo(start_ + 1, cache_.size());
    cache_[start_] = tipset;
    if (len_ < cache_.size()) {
      len_++;
    }

    return outcome::success();
  }

  outcome::result<void> TipsetCacheImpl::revert(const Tipset &tipset) {
    return revert_(tipset);
  }

  outcome::result<Tipset> TipsetCacheImpl::getNonNull(uint64_t height) {
    while (true) {
      OUTCOME_TRY(tipset, get(height++));

      if (tipset) {
        return tipset.get();
      }
    }
  }

  outcome::result<boost::optional<Tipset>> TipsetCacheImpl::get(
      uint64_t height) {
    if (len_ == 0) {
      OUTCOME_TRY(tipset, get_function_(height, TipsetKey()));
      OUTCOME_TRY(add(tipset));
      return std::move(tipset);
    }

    auto head_height = cache_[start_]->height;

    if (height > head_height) {
      return TipsetCacheError::kNotInCache;
    }

    auto cache_length = cache_.size();
    boost::optional<Tipset> tail = boost::none;
    uint64_t i;
    for (i = 1; i <= len_; i++) {
      tail = cache_[normalModulo(start_ - len_ + i, cache_length)];
      if (tail) {
        break;
      }
    }

    if (height < tail->height) {
      OUTCOME_TRY(key, tail->makeKey());
      OUTCOME_TRY(tipset, get_function_(height, key));
      if (i > (tail->height - height)) {
        cache_[normalModulo(start_ - len_ + (tail->height - height),
                            cache_length)] = tipset;
        // TODO: Maybe extend cache, if len less than cache size
      }
      return std::move(tipset);
    }

    return cache_[normalModulo(start_ - (head_height - height), cache_length)];
  }

  boost::optional<Tipset> TipsetCacheImpl::best() const {
    return cache_[start_];
  }

  outcome::result<void> TipsetCacheImpl::revert_(
      const boost::optional<Tipset> &maybe_tipset) {
    if (len_ == 0) {
      return outcome::success();
    }

    if (cache_[start_] != maybe_tipset) {
      return TipsetCacheError::kNotMatchHead;
    }

    cache_[start_] = boost::none;
    start_ = normalModulo(start_ - 1, cache_.size());
    len_--;

    // NOLINTNEXTLINE
    revert_(boost::none);  // remove previous boost::none entries
    return outcome::success();
  }

}  // namespace fc::mining

OUTCOME_CPP_DEFINE_CATEGORY(fc::mining, TipsetCacheError, e) {
  using fc::mining::TipsetCacheError;
  switch (e) {
    case (TipsetCacheError::kSmallerHeight):
      return "TipsetCache: cache height is higher than the new tipset";
    case (TipsetCacheError::kNotMatchHead):
      return "TipsetCache: revert tipset doesn't match cache head";
    case (TipsetCacheError::kNotInCache):
      return "TipsetCache: requested tipset not in cache";
    default:
      return "TipsetCache: unknown error";
  }
}
