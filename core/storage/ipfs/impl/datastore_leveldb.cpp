/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/impl/datastore_leveldb.hpp"

#include <libp2p/multi/content_identifier_codec.hpp>
#include <storage/leveldb/leveldb_error.hpp>

namespace fc::storage::ipfs {
  namespace {
    /**
     * @brief convenience function to encode value
     * @param value key value to encode
     * @return encoded value as Buffer
     */
    inline outcome::result<common::Buffer> encode(
        const CID &value) {
      OUTCOME_TRY(encoded,
                  libp2p::multi::ContentIdentifierCodec::encode(value));
      return common::Buffer(std::move(encoded));
    }
  }  // namespace

  LeveldbDatastore::LeveldbDatastore(std::shared_ptr<LevelDB> leveldb)
      : leveldb_{std::move(leveldb)} {
    BOOST_ASSERT_MSG(leveldb_ != nullptr, "leveldb argument is nullptr");
  }

  outcome::result<std::shared_ptr<LeveldbDatastore>> LeveldbDatastore::create(
      std::string_view leveldb_directory, leveldb::Options options) {
    OUTCOME_TRY(leveldb, LevelDB::create(leveldb_directory, options));

    return std::make_shared<LeveldbDatastore>(std::move(leveldb));
  }

  outcome::result<bool> LeveldbDatastore::contains(const CID &key) const {
    OUTCOME_TRY(encoded_key, encode(key));
    return leveldb_->contains(encoded_key);
  }

  outcome::result<void> LeveldbDatastore::set(const CID &key, Value value) {
    // TODO(turuslan): FIL-117 maybe check value hash matches cid
    OUTCOME_TRY(encoded_key, encode(key));
    return leveldb_->put(encoded_key, common::Buffer(std::move(value)));
  }

  outcome::result<LeveldbDatastore::Value> LeveldbDatastore::get(
      const CID &key) const {
    OUTCOME_TRY(encoded_key, encode(key));
    auto res = leveldb_->get(encoded_key);
    if (res.has_error() && res.error() == fc::storage::LevelDBError::NOT_FOUND)
      return fc::storage::ipfs::IpfsDatastoreError::NOT_FOUND;
    return res;
  }

  outcome::result<void> LeveldbDatastore::remove(const CID &key) {
    OUTCOME_TRY(encoded_key, encode(key));
    return leveldb_->remove(encoded_key);
  }

}  // namespace fc::storage::ipfs
