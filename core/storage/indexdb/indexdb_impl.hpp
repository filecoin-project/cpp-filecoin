/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_INDEXDB_IMPL_HPP
#define CPP_FILECOIN_STORAGE_INDEXDB_IMPL_HPP

#include "indexdb.hpp"

#include <libp2p/storage/sqlite.hpp>

#include "graph.hpp"

namespace fc::storage::indexdb {

  auto constexpr kLogName = "indexdb";

  using Blob = std::vector<uint8_t>;

  class IndexDbImpl : public IndexDb {
    using StatementHandle = libp2p::storage::SQLite::StatementHandle;

   public:
    explicit IndexDbImpl(const std::string &db_filename);

    outcome::result<void> initDb();

    outcome::result<void> loadGraph();

    Branches getHeads() override;

    BranchId getNewBranchId() override;

    outcome::result<std::reference_wrapper<const BranchInfo>> getBranchInfo(
        BranchId id) override;

    //bool containsTipset(const TipsetHash& hash) override;

    outcome::result<TipsetInfo> getTipsetInfo(const TipsetHash &hash) override;

    outcome::result<std::vector<CID>> getTipsetCIDs(
        const TipsetHash &hash) override;

    outcome::result<void> appendTipsetOnTop(const Tipset &tipset,
                                            BranchId branchId) override;

   private:
    /// RAII tx helper
    class Tx {
     public:
      explicit Tx(IndexDbImpl &db);
      void commit();
      void rollback();
      ~Tx();

     private:
      IndexDbImpl &db_;
      bool done_ = false;
    };

    [[nodiscard]] Tx beginTx();

    libp2p::storage::SQLite db_;
    StatementHandle get_tipset_info_{};
    StatementHandle get_tipset_blocks_{};
    StatementHandle insert_tipset_{};
    Graph graph_;
    BranchId branch_id_counter_ = 1;

    friend Tx;
  };

}  // namespace fc::storage::indexdb

#endif  // CPP_FILECOIN_STORAGE_INDEXDB_IMPL_HPP
