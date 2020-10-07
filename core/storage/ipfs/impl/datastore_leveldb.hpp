/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPFS_IMPL_DATASTORE_LEVELDB_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPFS_IMPL_DATASTORE_LEVELDB_HPP

#include <memory>

#include "common/outcome.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/leveldb/leveldb.hpp"

namespace fc::storage::ipfs {

  /**
   * @class LeveldbDatastore IpfsDatastore implementation based on LevelDB
   * database wrapper
   */
  class LeveldbDatastore
      : public IpfsDatastore,
        public std::enable_shared_from_this<LeveldbDatastore> {
   public:
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

    outcome::result<void> set(const CID &key, Value value) override;

    outcome::result<Value> get(const CID &key) const override;

    outcome::result<void> remove(const CID &key) override;

    IpldPtr shared() override {
      return shared_from_this();
    }

   private:
    std::shared_ptr<BufferMap> leveldb_;  ///< underlying db wrapper
  };

}  // namespace fc::storage::ipfs

#endif  // CPP_FILECOIN_CORE_STORAGE_IPFS_IMPL_DATASTORE_LEVELDB_HPP
