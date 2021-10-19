/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <mutex>
#include "common/bytes.hpp"
#include "common/outcome.hpp"
#include "storage/face/persistent_map.hpp"

namespace fc::primitives {
  using Datastore = storage::face::PersistentMap<Bytes, Bytes>;

  class Counter {
   public:
    virtual ~Counter() = default;

    virtual outcome::result<uint64_t> next() = 0;
  };

  class StoredCounter : public Counter {
   public:
    StoredCounter(std::shared_ptr<Datastore> datastore, const std::string &key);

    outcome::result<uint64_t> next() override;

    outcome::result<uint64_t> getNumber() const;

    outcome::result<void> setNumber(uint64_t number);

   private:
    std::shared_ptr<Datastore> datastore_;
    Bytes key_;

    mutable std::mutex mutex_;
  };
}  // namespace fc::primitives
