/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/impl/datastore_leveldb.hpp"

#include "codec/cbor/cbor.hpp"

namespace fc::storage::ipfs {
  namespace {
    /**
     * @brief convenience function to encode value
     * @param value key value to encode
     * @return encoded value as Buffer
     */
    inline common::Buffer encode(
        const libp2p::multi::ContentIdentifier &value) {
      return common::Buffer(codec::cbor::encode(value));
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

  bool LeveldbDatastore::contains(const CID &key) const {
    return leveldb_->contains(encode(key));
  }

  outcome::result<void> LeveldbDatastore::set(const CID &key, Value value) {
    return leveldb_->put(encode(key), common::Buffer(std::move(value)));
  }

  outcome::result<LeveldbDatastore::Value> LeveldbDatastore::get(
      const CID &key) const {
    return leveldb_->get(encode(key));
  }

  outcome::result<void> LeveldbDatastore::remove(const CID &key) {
    return leveldb_->remove(encode(key));
  }

}  // namespace fc::storage::ipfs
