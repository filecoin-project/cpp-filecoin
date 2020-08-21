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

  TipsetCacheImpl::TipsetCacheImpl(uint64_t capability) {
    cache_.reserve(capability);
    start_ = 0;
    len_ = 0;
    cache_[start_] = boost::none;
  }

  outcome::result<void> TipsetCacheImpl::add(const Tipset &tipset) {
    if (len_ > 0) {
      if (cache_[start_]->height >= tipset.height) {
        return outcome::success();  // TODO: Error
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
    if (len_ == 0) {
      return outcome::success();
    }

    if (cache_[start_].get() != tipset) {
      return outcome::success();  // TODO: Error
    }

    cache_[start_] = boost::none;
    start_ = normalModulo(start_ - 1, cache_.size());
    len_--;

    // TODO: remove null blocks

    return outcome::success();
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
      // TODO: download tipset
      // TODO: save
      return outcome::success();  // TODO: return Tipset
    }

    auto head_height = cache_[start_]->height;

    if (height > head_height) {
      return outcome::success();  // TODO: Error
    }

    auto cache_length = cache_.size();
    boost::optional<Tipset> tail = boost::none;
    for (uint64_t i = 1; i <= len_; i++) {
      tail = cache_[normalModulo(start_ - len_ + i, cache_length)];
      if (tail) {
        break;
      }
    }

    if (height < tail->height) {
      // TODO: download tipset
      // TODO: save if can be allocated
      return outcome::success();
    }

    return cache_[normalModulo(start_ - (head_height - height), cache_length)];
  }

  boost::optional<Tipset> TipsetCacheImpl::best() const {
    return cache_[start_];
  }

}  // namespace fc::mining
