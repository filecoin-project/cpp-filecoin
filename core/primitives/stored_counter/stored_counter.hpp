/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_STORED_COUNTER_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_STORED_COUNTER_HPP

#include <mutex>
#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "storage/face/persistent_map.hpp"

namespace fc::primitives {
  using Datastore = storage::face::PersistentMap<Buffer, Buffer>;

  class StoredCounter {
   public:
    StoredCounter(std::shared_ptr<Datastore> datastore, std::string key);

    outcome::result<uint64_t> next();

   private:
    std::shared_ptr<Datastore> datastore_;
    Buffer key_;

    std::mutex mutex_;
  };
}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_STORED_COUNTER_HPP
