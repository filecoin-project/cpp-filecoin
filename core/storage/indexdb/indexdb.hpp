/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_INDEXDB_HPP
#define CPP_FILECOIN_STORAGE_INDEXDB_HPP

#include "primitives/cid/cid.hpp"

namespace fc::storage::indexdb {

  enum class Error {
    INDEXDB_CANNOT_CREATE = 1,
    INDEXDB_INVALID_ARGUMENT,
    INDEXDB_EXECUTE_ERROR,
    INDEXDB_DECODE_ERROR
  };

  /// RAII tx helper
  class Tx {
   public:
    explicit Tx(class IndexDb &db);
    void commit();
    void rollback();
    ~Tx();

   private:
    class IndexDb &db_;
    bool done_ = false;
  };

  class IndexDb {
   public:
    using Blob = std::vector<uint8_t>;

    virtual ~IndexDb() = default;

    virtual Tx beginTx() = 0;
    virtual void commitTx() = 0;
    virtual void rollbackTx() = 0;

    virtual outcome::result<void> getParents(
        const Blob &id, const std::function<void(const Blob &)> &cb) = 0;

    virtual outcome::result<void> setParent(const Blob &parent,
                                            const Blob &child) = 0;
  };

  outcome::result<std::shared_ptr<IndexDb>> createIndexDb(
      const std::string &db_filename);

  outcome::result<void> setParent(IndexDb &db,
                                  const CID &parent,
                                  const CID &child);

  outcome::result<std::vector<CID>> getParents(IndexDb &db, const CID &cid);

}  // namespace fc::storage::indexdb

OUTCOME_HPP_DECLARE_ERROR(fc::storage::indexdb, Error);

#endif  // CPP_FILECOIN_STORAGE_INDEXDB_HPP
