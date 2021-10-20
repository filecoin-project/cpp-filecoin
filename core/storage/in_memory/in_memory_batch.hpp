/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/hexutil.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace fc::storage {

  class InMemoryBatch : public fc::storage::face::WriteBatch<Bytes, Bytes> {
   public:
    explicit InMemoryBatch(InMemoryStorage &db) : db{db} {}

    outcome::result<void> put(const Bytes &key, const Bytes &value) override {
      entries[common::hex_upper(key)] = value;
      return outcome::success();
    }

    outcome::result<void> put(const Bytes &key, Bytes &&value) override {
      entries[common::hex_upper(key)] = std::move(value);
      return outcome::success();
    }

    outcome::result<void> remove(const Bytes &key) override {
      entries.erase(common::hex_upper(key));
      return outcome::success();
    }

    outcome::result<void> commit() override {
      for (auto &entry : entries) {
        OUTCOME_TRY(db.put(common::unhex(entry.first).value(), entry.second));
      }
      return outcome::success();
    }

    void clear() override {
      entries.clear();
    }

   private:
    std::map<std::string, Bytes> entries;
    InMemoryStorage &db;
  };
}  // namespace fc::storage
