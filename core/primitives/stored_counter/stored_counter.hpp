/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <mutex>
#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "storage/face/persistent_map.hpp"

namespace fc::primitives {
  using Datastore = storage::face::PersistentMap<Buffer, Buffer>;

  class Counter {
   public:
    virtual ~Counter() = default;

    virtual outcome::result<uint64_t> next() = 0;
  };

  class StoredCounter : public Counter {
   public:
    StoredCounter(std::shared_ptr<Datastore> datastore, std::string key);

    outcome::result<uint64_t> next() override;

   private:
    std::shared_ptr<Datastore> datastore_;
    Buffer key_;

    std::mutex mutex_;
  };
}  // namespace fc::primitives
