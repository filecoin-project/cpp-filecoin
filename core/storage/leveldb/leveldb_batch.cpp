/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb_batch.hpp"

#include "storage/leveldb/leveldb_util.hpp"

namespace fc::storage {

  LevelDB::Batch::Batch(LevelDB &db) : db_(db) {}

  outcome::result<void> LevelDB::Batch::put(const Bytes &key,
                                            BytesCow &&value) {
    batch_.Put(make_slice(key), make_slice(value));
    return outcome::success();
  }

  outcome::result<void> LevelDB::Batch::remove(const Bytes &key) {
    batch_.Delete(make_slice(key));
    return outcome::success();
  }

  outcome::result<void> LevelDB::Batch::commit() {
    auto status = db_.db_->Write(db_.wo_, &batch_);
    if (status.ok()) {
      return outcome::success();
    }

    return error_as_result<void>(status, db_.logger_);
  }

  void LevelDB::Batch::clear() {
    batch_.Clear();
  }

}  // namespace fc::storage
