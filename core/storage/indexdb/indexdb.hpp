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

    virtual outcome::result<std::reference_wrapper<const BranchInfo>>
    getBranchInfo(BranchId id) = 0;

    //virtual bool containsTipset(const TipsetHash& hash) = 0;

    virtual outcome::result<TipsetInfo> getTipsetInfo(
        const TipsetHash &hash) = 0;

    virtual outcome::result<std::vector<CID>> getTipsetCIDs(
        const TipsetHash &hash) = 0;

    /// Appends tipset on top of existing branch
    virtual outcome::result<void> appendTipsetOnTop(const Tipset &tipset,
                                                     BranchId branchId) = 0;
  };

}  // namespace fc::storage::indexdb

#endif  // CPP_FILECOIN_STORAGE_INDEXDB_HPP
