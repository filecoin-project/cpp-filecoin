/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "storage/buffer_map.hpp"

namespace fc::storage {

  using fc::common::Buffer;
  /**
   * Simple storage that conforms PersistentMap interface
   * Mostly needed to have an in-memory trie in tests to avoid integration with
   * LevelDB
   */
  class InMemoryStorage : public PersistentBufferMap,
                          public std::enable_shared_from_this<InMemoryStorage> {
   public:
    ~InMemoryStorage() override = default;

    outcome::result<Buffer> get(const Buffer &key) const override;

    outcome::result<void> put(const Buffer &key, const Buffer &value) override;

    outcome::result<void> put(const Buffer &key, Buffer &&value) override;

    bool contains(const Buffer &key) const override;

    outcome::result<void> remove(const Buffer &key) override;

    std::unique_ptr<fc::storage::face::WriteBatch<Buffer, Buffer>> batch()
        override;

    std::unique_ptr<BufferMapCursor> cursor() override;

   private:
    friend class InMemoryCursor;
    std::map<std::string, Buffer> storage;
  };

}  // namespace fc::storage
