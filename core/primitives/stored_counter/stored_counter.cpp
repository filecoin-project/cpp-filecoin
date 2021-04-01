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

    outcome::result<uint64_t> StoredCounter::next() {
        std::lock_guard lock(mutex_);

        auto has = datastore_->contains(key_);

        uint64_t next = 0;
        if (has) {
            OUTCOME_TRY(value, datastore_->get(key_));
            libp2p::multi::UVarint cur(value);
            next = cur.toUInt64() + 1;
        }
        libp2p::multi::UVarint new_value(next);
        OUTCOME_TRY(datastore_->put(key_, Buffer(new_value.toBytes())));

        return next;
    }
}  // namespace fc::primitives
