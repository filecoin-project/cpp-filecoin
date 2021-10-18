/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "common/outcome.hpp"
#include "storage/buffer_map.hpp"

namespace fc::storage {
  /**
   * Simple storage that conforms PersistentMap interface
   * Mostly needed to have an in-memory trie in tests to avoid integration with
   * LevelDB
   */
  class InMemoryStorage : public PersistentBufferMap,
                          public std::enable_shared_from_this<InMemoryStorage> {
   public:
    ~InMemoryStorage() override = default;

    outcome::result<Bytes> get(const Bytes &key) const override;

    outcome::result<void> put(const Bytes &key, const Bytes &value) override;

    outcome::result<void> put(const Bytes &key, Bytes &&value) override;

    bool contains(const Bytes &key) const override;

    outcome::result<void> remove(const Bytes &key) override;

    std::unique_ptr<fc::storage::face::WriteBatch<Bytes, Bytes>> batch()
        override;

    std::unique_ptr<BufferMapCursor> cursor() override;

   private:
    friend class InMemoryCursor;
    std::map<std::string, Bytes> storage;
  };

}  // namespace fc::storage
