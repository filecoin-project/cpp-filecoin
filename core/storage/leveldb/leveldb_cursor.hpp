/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_LEVELDB_CURSOR_HPP
#define FILECOIN_LEVELDB_CURSOR_HPP

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

    void seek(const Buffer &key) override;

    void seekToLast() override;

    bool isValid() const override;

    void next() override;

    void prev() override;

    Buffer key() const override;

    Buffer value() const override;

   private:
    std::shared_ptr<leveldb::Iterator> i_;
  };

}  // namespace fc::storage

#endif  // FILECOIN_LEVELDB_CURSOR_HPP
