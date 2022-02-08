/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/stored_counter/stored_counter.hpp"

#include <libp2p/multi/uvarint.hpp>

#include "codec/uvarint.hpp"
#include "common/span.hpp"

namespace fc::primitives {
  StoredCounter::StoredCounter(std::shared_ptr<Datastore> datastore,
                               const std::string &key)
      : datastore_(std::move(datastore)),
        key_(copy(fc::common::span::cbytes(key))) {}

  outcome::result<uint64_t> StoredCounter::getNumber() const {
    std::lock_guard lock(mutex_);
    return getNumberWithoutLock();
  }

  outcome::result<uint64_t> StoredCounter::getNumberWithoutLock() const {
    if (not datastore_->contains(key_)) {
      return 0;
    }
    OUTCOME_TRY(value, datastore_->get(key_));
    const libp2p::multi::UVarint cur(value);
    return cur.toUInt64();
  }

  outcome::result<void> StoredCounter::setNumber(uint64_t number) {
    std::lock_guard lock(mutex_);
    return setNumberWithoutLock(number);
  }

  outcome::result<void> StoredCounter::setNumberWithoutLock(uint64_t number) {
    return datastore_->put(key_, codec::uvarint::VarintEncoder{number}.bytes());
  }

  outcome::result<uint64_t> StoredCounter::next() {
    std::lock_guard lock(mutex_);

    OUTCOME_TRY(value, getNumberWithoutLock());
    OUTCOME_TRY(setNumberWithoutLock(value + 1));

    return value;
  }
}  // namespace fc::primitives
