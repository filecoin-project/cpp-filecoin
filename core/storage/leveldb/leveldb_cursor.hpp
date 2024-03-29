/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <leveldb/iterator.h>
#include "storage/leveldb/leveldb.hpp"

namespace fc::storage {

  /**
   * @brief Instance of cursor can be used as bidirectional iterator over
   * key-value bindings of the Map.
   */
  class LevelDB::Cursor : public BufferMapCursor {
   public:
    ~Cursor() override = default;

    explicit Cursor(std::shared_ptr<leveldb::Iterator> it);

    void seekToFirst() override;

    void seek(const Bytes &key) override;

    void seekToLast() override;

    bool isValid() const override;

    void next() override;

    void prev() override;

    Bytes key() const override;

    Bytes value() const override;

   private:
    std::shared_ptr<leveldb::Iterator> i_;
  };

}  // namespace fc::storage
