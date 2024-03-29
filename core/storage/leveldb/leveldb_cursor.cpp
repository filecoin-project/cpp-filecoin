/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb_cursor.hpp"

#include "storage/leveldb/leveldb_util.hpp"

namespace fc::storage {

  LevelDB::Cursor::Cursor(std::shared_ptr<leveldb::Iterator> it)
      : i_(std::move(it)) {}

  void LevelDB::Cursor::seekToFirst() {
    i_->SeekToFirst();
  }

  void LevelDB::Cursor::seek(const Bytes &key) {
    i_->Seek(make_slice(key));
  }

  void LevelDB::Cursor::seekToLast() {
    i_->SeekToLast();
  }

  bool LevelDB::Cursor::isValid() const {
    return i_->Valid();
  }

  void LevelDB::Cursor::next() {
    i_->Next();
  }

  void LevelDB::Cursor::prev() {
    i_->Prev();
  }

  Bytes LevelDB::Cursor::key() const {
    return make_buffer(i_->key());
  }

  Bytes LevelDB::Cursor::value() const {
    return make_buffer(i_->value());
  }

}  // namespace fc::storage
