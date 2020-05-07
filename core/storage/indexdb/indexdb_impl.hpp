/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_INDEXDB_IMPL_HPP
#define CPP_FILECOIN_STORAGE_INDEXDB_IMPL_HPP

#include "indexdb.hpp"

#include <libp2p/storage/sqlite.hpp>

#include <common/logger.hpp>

namespace fc::storage::indexdb {

  class IndexDbImpl : public IndexDb {
    using StatementHandle = libp2p::storage::SQLite::StatementHandle;

   public:

    explicit IndexDbImpl(const std::string &db_filename);

    Tx beginTx() override;

    void commitTx() override;

    void rollbackTx() override;

    outcome::result<void> getParents(
        const Blob &id, const std::function<void(const Blob &)> &cb) override;

    outcome::result<void> setParent(const Blob &parent, const Blob &child) override;

   private:
    void init();

    libp2p::storage::SQLite db_;
    StatementHandle get_parents_{};
    StatementHandle set_parent_{};
    StatementHandle get_successors_{};
  };

} // namespace fc::storage::indexdb

#endif  // CPP_FILECOIN_STORAGE_INDEXDB_IMPL_HPP
