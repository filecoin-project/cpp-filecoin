/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb.hpp"

#include <utility>

#include "common/span.hpp"
#include "storage/leveldb/leveldb_batch.hpp"
#include "storage/leveldb/leveldb_cursor.hpp"
#include "storage/leveldb/leveldb_util.hpp"

namespace fc::storage {

  outcome::result<std::shared_ptr<LevelDB>> LevelDB::create(
      std::string_view path, leveldb::Options options) {
    leveldb::DB *db = nullptr;
    auto status = leveldb::DB::Open(options, path.data(), &db);
    if (status.ok()) {
      auto l = std::make_shared<LevelDB>();
      l->db_ = std::unique_ptr<leveldb::DB>(db);
      return std::move(l);  // clang 6.0.1 issue
    }

    return error_as_result<std::shared_ptr<LevelDB>>(status);
  }

  outcome::result<std::shared_ptr<LevelDB>> LevelDB::create(
      std::string_view path) {
    leveldb::Options options;
    options.create_if_missing = true;
    return create(path, options);
  }

  std::unique_ptr<BufferMapCursor> LevelDB::cursor() {
    auto it = std::unique_ptr<leveldb::Iterator>(db_->NewIterator(ro_));
    return std::make_unique<Cursor>(std::move(it));
  }

  std::unique_ptr<BufferBatch> LevelDB::batch() {
    return std::make_unique<Batch>(*this);
  }

  void LevelDB::setReadOptions(leveldb::ReadOptions ro) {
    ro_ = ro;
  }

  void LevelDB::setWriteOptions(leveldb::WriteOptions wo) {
    wo_ = wo;
  }

  outcome::result<Bytes> LevelDB::get(const Bytes &key) const {
    std::string value;
    auto status = db_->Get(ro_, make_slice(key), &value);
    if (status.ok()) {
      // FIXME: is it possible to avoid copying string -> Bytes?
      return copy(common::span::cbytes(value));
    }

    return error_as_result<Bytes>(status, logger_);
  }

  bool LevelDB::contains(const Bytes &key) const {
    // here we interpret all kinds of errors as "not found".
    // is there a better way?
    std::string value;
    return db_->Get(ro_, make_slice(key), &value).ok();
  }

  outcome::result<void> LevelDB::put(const Bytes &key, const Bytes &value) {
    return put(key, gsl::make_span(value));
  }

  outcome::result<void> LevelDB::put(const Bytes &key, Bytes &&value) {
    return put(key, gsl::make_span(value));
  }

  outcome::result<void> LevelDB::put(const Bytes &key, BytesIn value) {
    auto status = db_->Put(wo_, make_slice(key), make_slice(value));
    if (status.ok()) {
      return outcome::success();
    }

    return error_as_result<void>(status, logger_);
  }

  outcome::result<void> LevelDB::remove(const Bytes &key) {
    auto status = db_->Delete(wo_, make_slice(key));
    if (status.ok()) {
      return outcome::success();
    }

    return error_as_result<void>(status, logger_);
  }

}  // namespace fc::storage
