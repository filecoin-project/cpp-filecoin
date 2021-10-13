/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/stored_counter/stored_counter.hpp"

#include <libp2p/multi/uvarint.hpp>
#include "common/span.hpp"

namespace fc::primitives {
  StoredCounter::StoredCounter(std::shared_ptr<Datastore> datastore,
                               std::string key)
      : datastore_(std::move(datastore)), key_(fc::common::span::cbytes(key)) {}

  outcome::result<uint64_t> StoredCounter::getNumber() const {
    std::lock_guard lock(mutex_);
    if (not datastore_->contains(key_)) {
      return 0;
    }
    OUTCOME_TRY(value, datastore_->get(key_));
    const libp2p::multi::UVarint cur(value);
    return cur.toUInt64();
  }

  outcome::result<void> StoredCounter::setNumber(uint64_t number) {
    std::lock_guard lock(mutex_);
    const libp2p::multi::UVarint new_value(number);
    return datastore_->put(key_, Buffer(new_value.toBytes()));
  }

  outcome::result<uint64_t> StoredCounter::next() {
    std::lock_guard lock(mutex_);

    OUTCOME_TRY(value, getNumber());
    OUTCOME_TRY(setNumber(value + 1));

    return value;
  }
}  // namespace fc::primitives
