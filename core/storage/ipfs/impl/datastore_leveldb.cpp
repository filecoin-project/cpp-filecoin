/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/impl/datastore_leveldb.hpp"

#include "storage/leveldb/leveldb_error.hpp"

namespace fc::storage::ipfs {
  outcome::result<Bytes> LeveldbDatastore::encodeKey(const CID &value) {
    return value.toBytes();
  }

  LeveldbDatastore::LeveldbDatastore(std::shared_ptr<BufferMap> leveldb)
      : leveldb_{std::move(leveldb)} {
    BOOST_ASSERT_MSG(leveldb_ != nullptr, "leveldb argument is nullptr");
  }

  outcome::result<std::shared_ptr<LeveldbDatastore>> LeveldbDatastore::create(
      std::string_view leveldb_directory, leveldb::Options options) {
    OUTCOME_TRY(leveldb, LevelDB::create(leveldb_directory, options));

    return std::make_shared<LeveldbDatastore>(std::move(leveldb));
  }

  outcome::result<bool> LeveldbDatastore::contains(const CID &key) const {
    OUTCOME_TRY(encoded_key, encodeKey(key));
    return leveldb_->contains(encoded_key);
  }

  outcome::result<void> LeveldbDatastore::set(const CID &key,
                                              BytesCow &&value) {
    // TODO(turuslan): FIL-117 maybe check value hash matches cid
    OUTCOME_TRY(encoded_key, encodeKey(key));
    return leveldb_->put(encoded_key, std::move(value));
  }

  outcome::result<LeveldbDatastore::Value> LeveldbDatastore::get(
      const CID &key) const {
    OUTCOME_TRY(encoded_key, encodeKey(key));
    auto res = leveldb_->get(encoded_key);
    if (res.has_error() && res.error() == fc::storage::LevelDBError::kNotFound)
      return fc::storage::ipfs::IpfsDatastoreError::kNotFound;
    return res;
  }
}  // namespace fc::storage::ipfs
