/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "common/outcome.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/leveldb/leveldb.hpp"

namespace fc::storage::ipfs {

  /**
   * @class LeveldbDatastore IpfsDatastore implementation based on LevelDB
   * database wrapper
   */
  class LeveldbDatastore : public Ipld {
   public:
    /**
     * @brief convenience function to encode value
     * @param value key value to encode
     * @return encoded value as Bytes
     */
    static outcome::result<Bytes> encodeKey(const CID &value);

    /**
     * @brief constructor
     * @param leveldb shared pointer to leveldb instance
     */
    explicit LeveldbDatastore(std::shared_ptr<BufferMap> leveldb);

    ~LeveldbDatastore() override = default;

    /**
     * @brief creates LeveldbDatastore instance
     * @param leveldb_directory path to leveldb directory
     * @param options leveldb database options
     * @return shared pointer to instance
     */
    static outcome::result<std::shared_ptr<LeveldbDatastore>> create(
        std::string_view leveldb_directory, leveldb::Options options);

    outcome::result<bool> contains(const CID &key) const override;

    outcome::result<void> set(const CID &key, BytesCow &&value) override;

    outcome::result<Value> get(const CID &key) const override;

   private:
    std::shared_ptr<BufferMap> leveldb_;  ///< underlying db wrapper
  };

}  // namespace fc::storage::ipfs
