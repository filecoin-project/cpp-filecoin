/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_INDEX_DB_BACKEND_HPP
#define CPP_FILECOIN_SYNC_INDEX_DB_BACKEND_HPP

#include <libp2p/storage/sqlite.hpp>

#include "index_db.hpp"

namespace fc::sync {

  /// Persistent (sqlite) index db backend
  class IndexDbBackend {
   public:
    enum class Error {
      INDEXDB_CANNOT_CREATE = 1,
      INDEXDB_DATA_INTEGRITY_ERROR,
      INDEXDB_ALREADY_EXISTS,
      INDEXDB_EXECUTE_ERROR,
      INDEXDB_TIPSET_NOT_FOUND,
    };

    /// Just to utilize sqliet_modern_cpp lib, but this is suboptimal
    using Blob = std::vector<uint8_t>;

    /// Creates or opens sqlite db
    static outcome::result<std::shared_ptr<IndexDbBackend>> create(
        const std::string &db_filename);

    /// Initializes statements, reads graph info from db
    outcome::result<std::map<BranchId, std::shared_ptr<BranchInfo>>> initDb();

    /// Stores new tipset index and (optionally) renames branches, the latter
    /// occurs when mergin or splitting branches in graph
    outcome::result<void> store(
        const TipsetInfo &info,
        const boost::optional<RenameBranch> &branch_rename);

    /// Index internal repe
    struct TipsetIdx {
      /// Tipste hash
      TipsetHash hash;

      /// Tipset branch ID
      BranchId branch = kNoBranch;

      /// Tipset height
      Height height = 0;

      /// Hash of parent tipset
      TipsetHash parent_hash;

      /// Cids compressed
      Blob cids;
    };

    /// Returns tipset index by hash, if error_if_not_found==true then 'not
    /// found' condition leads to error outcome, otherwise nullptr
    outcome::result<TipsetIdx> get(const TipsetHash &hash,
                                   bool error_if_not_found);

    /// Returns tipset index by branch and height
    outcome::result<TipsetIdx> get(BranchId branch, Height height);

    /// Decodes interbnal repr to external
    static outcome::result<std::shared_ptr<TipsetInfo>> decode(TipsetIdx raw);

    /// Walks through index entries within a given branch, from given height,
    /// limited by given limit (SELECT query inside)
    outcome::result<void> walk(BranchId branch,
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

    /// Begins a TX, return RAII object
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

OUTCOME_HPP_DECLARE_ERROR(fc::sync, IndexDbBackend::Error);

#endif  // CPP_FILECOIN_SYNC_INDEX_DB_BACKEND_HPP
