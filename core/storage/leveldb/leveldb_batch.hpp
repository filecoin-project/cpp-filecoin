/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <leveldb/write_batch.h>
#include "storage/leveldb/leveldb.hpp"

namespace fc::storage {

  /**
   * @brief Class that is used to implement efficient bulk (batch) modifications
   * of the Map.
   */
  class LevelDB::Batch : public BufferBatch {
   public:
    explicit Batch(LevelDB &db);

    outcome::result<void> put(const Bytes &key, const Bytes &value) override;
    outcome::result<void> put(const Bytes &key, Bytes &&value) override;

    outcome::result<void> remove(const Bytes &key) override;

    outcome::result<void> commit() override;

    void clear() override;

   private:
    LevelDB &db_;
    leveldb::WriteBatch batch_;
  };

}  // namespace fc::storage
