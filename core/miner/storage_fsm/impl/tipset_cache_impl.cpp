/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/tipset_cache_impl.hpp"

#include "primitives/tipset/tipset.hpp"

namespace fc::mining {
  TipsetCacheImpl::TipsetCacheImpl(uint64_t capability,
                                   std::shared_ptr<FullNodeApi> api)
      : api_(std::move(api)) {
    cache_.resize(capability);
    start_ = 0;
    len_ = 0;
  }

  outcome::result<void> TipsetCacheImpl::add(const Tipset &tipset) {
    std::unique_lock lock(mutex_);

    if (len_ > 0) {
      if (cache_[start_]->height() >= tipset.height()) {
        return TipsetCacheError::kSmallerHeight;
      }
    }

    auto current_height = tipset.height();
    if (len_ > 0) {
      current_height = cache_[start_]->height();
    }

    while (++current_height < tipset.height()) {
      start_ = mod(start_ + 1);
      cache_[start_] = boost::none;
      if (len_ < cache_.size()) {
        len_++;
      }
    }

    start_ = mod(start_ + 1);
    cache_[start_] = tipset;
    if (len_ < cache_.size()) {
      len_++;
    }

    return outcome::success();
  }

  outcome::result<void> TipsetCacheImpl::revert(const Tipset &tipset) {
    std::unique_lock lock(mutex_);

    if (len_ == 0) {
      return outcome::success();
    }

    if (cache_[start_] != tipset) {
      return TipsetCacheError::kNotMatchHead;
    }

    cache_[start_] = boost::none;
    start_ = mod(start_ - 1);
    len_--;

    while (len_ && cache_[start_] == boost::none) {
      start_ = mod(start_ - 1);
      len_--;
    }
    return outcome::success();
  }

  outcome::result<Tipset> TipsetCacheImpl::getNonNull(ChainEpoch height) {
    while (true) {
      OUTCOME_TRY(tipset, get(height++));

      if (tipset) {
        return tipset.get();
      }
    }
  }

  outcome::result<boost::optional<Tipset>> TipsetCacheImpl::get(
      ChainEpoch height) {
    std::shared_lock lock(mutex_);

    if (len_ == 0) {
      OUTCOME_TRY(tipset, api_->ChainGetTipSetByHeight(height, {}));
      return *tipset;
    }

    auto head_height = cache_[start_]->height();

    if (height > head_height) {
      return TipsetCacheError::kNotInCache;
    }

    boost::optional<Tipset> tail = boost::none;
    uint64_t i;
    for (i = 1; i <= len_; i++) {
      tail = cache_[mod(start_ - len_ + i)];
      if (tail) {
        break;
      }
    }

    if (height < tail->height()) {
      OUTCOME_TRY(tipset, api_->ChainGetTipSetByHeight(height, {}));
      return *tipset;
    }

    return cache_[mod(start_ - (head_height - height))];
  }

  outcome::result<Tipset> TipsetCacheImpl::best() const {
    std::shared_lock lock(mutex_);
    if (len_ == 0) {
      OUTCOME_TRY(tipset, api_->ChainHead());
      return *tipset;
    }
    return cache_[start_].value();
  }

  int64_t TipsetCacheImpl::mod(int64_t x) {
    auto size = cache_.size();
    return ((x % size) + size) % size;
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
