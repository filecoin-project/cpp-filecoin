/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STORAGE_INDEXDB_HPP
#define CPP_FILECOIN_STORAGE_INDEXDB_HPP

#include "common.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::storage::indexdb {

  using Tipset = primitives::tipset::Tipset;

  outcome::result<std::shared_ptr<class IndexDb>> createIndexDb(
      const std::string &db_filename);

  class IndexDb {
   public:
    virtual ~IndexDb() = default;

    virtual Branches getHeads() = 0;

    virtual BranchId getNewBranchId() = 0;

    virtual bool branchExists(BranchId id) = 0;

    virtual outcome::result<BranchInfo> getBranchInfo(BranchId id) = 0;

    virtual bool tipsetExists(const TipsetHash &hash) = 0;

    virtual outcome::result<TipsetInfo> getTipsetInfo(
        const TipsetHash &hash) = 0;

    virtual outcome::result<std::vector<CID>> getTipsetCIDs(
        const TipsetHash &hash) = 0;

    virtual outcome::result<TipsetKey> getParentTipsetKey(
        const TipsetHash &hash) = 0;

    struct UnsyncedRoots {
      boost::optional<TipsetHash> last_loaded;
      TipsetKey to_load;
      BranchId branch = kNoBranch;
      Height height = 0;
    };

    virtual outcome::result<UnsyncedRoots> getUnsyncedRootsOf(
        const TipsetHash &hash) = 0;

    struct TipsetFullInfo {
      Tipset tipset;
      TipsetInfo info;
    };

    virtual outcome::result<TipsetFullInfo> getTipsetFullInfo(
        const TipsetHash &hash) = 0;

    virtual outcome::result<void> loadChain(
        TipsetHash root,
        std::function<void(TipsetHash hash, std::vector<CID> cids)>
            callback) = 0;

    virtual outcome::result<void> writeGenesis(const Tipset &tipset) = 0;

    struct ApplyResult {
      bool on_top_of_branch = false;
      bool on_bottom_of_branch = false;
      bool branches_merged = false;
      bool branches_splitted = false;
      bool root_is_genesis = false;
      BranchId this_branch = kNoBranch;
      BranchId parent_branch = kNoBranch;
      BranchId splitted_branch = kNoBranch;
    };

    virtual outcome::result<ApplyResult> applyTipset(
        const Tipset &tipset,
        bool parent_must_exist,
        boost::optional<TipsetHash> parent,
        boost::optional<TipsetHash> successor) = 0;

    virtual outcome::result<void> eraseChain(const TipsetHash &from) = 0;
  };

}  // namespace fc::storage::indexdb

#endif  // CPP_FILECOIN_STORAGE_INDEXDB_HPP
