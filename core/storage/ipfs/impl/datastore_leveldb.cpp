/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/impl/datastore_leveldb.hpp"

#include "codec/cbor/cbor.hpp"

namespace fc::storage::ipfs {

  LeveldbDatastore::LeveldbDatastore(std::shared_ptr<LevelDB> leveldb)
      : leveldb_{std::move(leveldb)} {
    BOOST_ASSERT_MSG(leveldb_ != nullptr, "leveldb argument is nullptr");
  }

  outcome::result<std::shared_ptr<LeveldbDatastore>> LeveldbDatastore::create(
      std::string_view leveldb_directory, leveldb::Options options) {
    OUTCOME_TRY(leveldb, LevelDB::create(leveldb_directory, options));

    return std::make_shared<LeveldbDatastore>(std::move(leveldb));
  }

  bool LeveldbDatastore::contains(const CID &key) {
    return leveldb_->contains(encode(key));
  }

  outcome::result<void> LeveldbDatastore::set(const CID &key, Value value) {
    return leveldb_->put(encode(key), common::Buffer(std::move(value)));
  }

  outcome::result<LeveldbDatastore::Value> LeveldbDatastore::get(
      const CID &key) {
    return leveldb_->get(encode(key));
  }

  outcome::result<void> LeveldbDatastore::remove(const CID &key) {
    return leveldb_->remove(encode(key));
  }

  common::Buffer LeveldbDatastore::encode(const CID &value) {
    return common::Buffer(codec::cbor::encode(value));
  }

}  // namespace fc::storage::ipfs
