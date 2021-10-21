/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/in_memory/in_memory_storage.hpp"

#include "storage/in_memory/in_memory_batch.hpp"
#include "storage/in_memory/in_memory_cursor.hpp"

namespace fc::storage {

  outcome::result<Bytes> InMemoryStorage::get(const Bytes &key) const {
    if (storage.find(key) != storage.end()) {
      return storage.at(key);
    }
    return Bytes{};
  }

  outcome::result<void> InMemoryStorage::put(const Bytes &key,
                                             const Bytes &value) {
    storage[key] = value;
    return outcome::success();
  }

  outcome::result<void> InMemoryStorage::put(const Bytes &key, Bytes &&value) {
    storage[key] = std::move(value);
    return outcome::success();
  }

  bool InMemoryStorage::contains(const Bytes &key) const {
    return storage.find(key) != storage.end();
  }

  outcome::result<void> InMemoryStorage::remove(const Bytes &key) {
    storage.erase(key);
    return outcome::success();
  }

  std::unique_ptr<fc::storage::face::WriteBatch<Bytes, Bytes>>
  InMemoryStorage::batch() {
    return std::make_unique<InMemoryBatch>(*this);
  }

  std::unique_ptr<BufferMapCursor> InMemoryStorage::cursor() {
    return std::make_unique<InMemoryCursor>(shared_from_this());
  }
}  // namespace fc::storage
