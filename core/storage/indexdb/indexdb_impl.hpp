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

//    outcome::result<void> applyToTop(const Tipset &tipset,
//                                            BranchId branchId) override;
//
//    outcome::result<void> applyToBottom(const Tipset &tipset,
//                                                BranchId branchId) override;
//
//    outcome::result<void> linkBranches(BranchId parent,
//                                               BranchId top) override;

    outcome::result<ApplyResult> applyTipset(
        const Tipset &tipset,
        bool parent_must_exist,
        boost::optional<const TipsetHash &> parent,
        boost::optional<const TipsetHash &> successor) override;

    outcome::result<void> eraseChain(const TipsetHash& from) override;

    outcome::result<void> loadChain(
        TipsetHash root,
        std::function<void(TipsetHash hash, std::vector<CID> cids)>
        callback) override;

    outcome::result<void> writeGenesis(const Tipset &tipset) override;

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
    StatementHandle rename_branch_{};
    StatementHandle rename_parent_branch_{};
    Graph graph_;
    BranchId branch_id_counter_ = kGenesisBranch;

    friend Tx;
  };

}  // namespace fc::storage::indexdb

#endif  // CPP_FILECOIN_STORAGE_INDEXDB_IMPL_HPP
