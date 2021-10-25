/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/in_memory/in_memory_storage.hpp"

namespace fc::storage {

  class InMemoryBatch : public fc::storage::face::WriteBatch<Bytes, Bytes> {
   public:
    explicit InMemoryBatch(InMemoryStorage &db) : db{db} {}

    outcome::result<void> put(const Bytes &key, BytesCow &&value) override {
      entries[key] = value.into();
      return outcome::success();
    }

    outcome::result<void> remove(const Bytes &key) override {
      entries.erase(key);
      return outcome::success();
    }

    outcome::result<void> commit() override {
      for (auto &entry : entries) {
        OUTCOME_TRY(db.put(entry.first, std::move(entry.second)));
      }
      return outcome::success();
    }

    void clear() override {
      entries.clear();
    }

   private:
    std::map<Bytes, Bytes> entries;
    InMemoryStorage &db;
  };
}  // namespace fc::storage
