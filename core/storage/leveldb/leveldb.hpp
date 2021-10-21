/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include "common/logger.hpp"
#include "storage/buffer_map.hpp"

namespace fc::storage {

  /**
   * @brief An implementation of PersistentBufferMap interface, which uses
   * LevelDB as underlying storage.
   */
  class LevelDB : public PersistentBufferMap {
   public:
    class Batch;
    class Cursor;

    ~LevelDB() override = default;

    /**
     * @brief Factory method to create an instance of LevelDB class.
     * @param path filesystem path where database is going to be
     * @param options leveldb options, such as caching, logging, etc.
     * @return instance of LevelDB
     */
    static outcome::result<std::shared_ptr<LevelDB>> create(
        std::string_view path, leveldb::Options options);
    static outcome::result<std::shared_ptr<LevelDB>> create(
        std::string_view path);

    /**
     * @brief Set read options, which are used in @see LevelDB#get
     * @param ro options
     */
    void setReadOptions(leveldb::ReadOptions ro);

    /**
     * @brief Set write options, which are used in @see LevelDB#put
     * @param wo options
     */
    void setWriteOptions(leveldb::WriteOptions wo);

    std::unique_ptr<BufferMapCursor> cursor() override;

    std::unique_ptr<BufferBatch> batch() override;

    outcome::result<Bytes> get(const Bytes &key) const override;

    bool contains(const Bytes &key) const override;

    outcome::result<void> put(const Bytes &key, const Bytes &value) override;

    // value will be copied, not moved, due to internal structure of LevelDB
    outcome::result<void> put(const Bytes &key, Bytes &&value) override;

    outcome::result<void> put(const Bytes &key, BytesIn value) override;

    outcome::result<void> remove(const Bytes &key) override;

   private:
    std::unique_ptr<leveldb::DB> db_;
    leveldb::ReadOptions ro_;
    leveldb::WriteOptions wo_;
    common::Logger logger_ = common::createLogger("leveldb");
  };

}  // namespace fc::storage
