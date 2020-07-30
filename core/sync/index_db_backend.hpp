/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_INDEX_DB_BACKEND_HPP
#define CPP_FILECOIN_SYNC_INDEX_DB_BACKEND_HPP

#include <libp2p/storage/sqlite.hpp>

#include "index_db.hpp"

namespace fc::sync {

  class IndexDbBackend {
   public:
    using Blob = std::vector<uint8_t>;

    static outcome::result<std::shared_ptr<IndexDbBackend>> create(
        const std::string &db_filename);

    outcome::result<std::map<BranchId, std::shared_ptr<BranchInfo>>> initDb();

    outcome::result<void> store(
        const TipsetInfo &info,
        const boost::optional<RenameBranch> &branch_rename);

    struct TipsetIdx {
      TipsetHash hash;
      BranchId branch = kNoBranch;
      Height height = 0;
      TipsetHash parent_hash;
      Blob cids;
    };

    outcome::result<TipsetIdx> get(const TipsetHash &hash);

    outcome::result<TipsetIdx> get(BranchId branch, Height height);

    static outcome::result<std::shared_ptr<TipsetInfo>> decode(TipsetIdx raw);

    outcome::result<void> walk(
        BranchId branch,
        Height height,
        uint64_t limit,
        const std::function<void(TipsetIdx)> &cb);

    /// RAII tx helper
    class Tx {
     public:
      explicit Tx(IndexDbBackend &db);
      void commit();
      void rollback();
      ~Tx();

     private:
      IndexDbBackend &db_;
      bool done_ = false;
    };

    [[nodiscard]] Tx beginTx();

    explicit IndexDbBackend(const std::string &db_filename);
   private:
    using StatementHandle = libp2p::storage::SQLite::StatementHandle;

    libp2p::storage::SQLite db_;
    StatementHandle get_by_hash_{};
    StatementHandle get_by_position_{};
    StatementHandle insert_tipset_{};
    StatementHandle rename_branch_{};

    friend Tx;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_INDEX_DB_BACKEND_HPP
