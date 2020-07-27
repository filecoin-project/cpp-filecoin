/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_LOCAL_STORE_HPP
#define CPP_FILECOIN_LOCAL_STORE_HPP

#include "sector_storage/stores/store.hpp"

#include <shared_mutex>
#include "common/logger.hpp"
#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage::stores {

  // TODO(artyom-yurin): [FIL-231] Health Report for storages
  class LocalStoreImpl : public LocalStore {
   public:
    static outcome::result<std::unique_ptr<LocalStore>> newLocalStore(
        const std::shared_ptr<LocalStorage> &storage,
        const std::shared_ptr<SectorIndex> &index,
        gsl::span<const std::string> urls);

    outcome::result<void> openPath(const std::string &path) override;

    outcome::result<AcquireSectorResponse> acquireSector(
        SectorId sector,
        RegisteredProof seal_proof_type,
        SectorFileType existing,
        SectorFileType allocate,
        bool can_seal) override;

    outcome::result<void> remove(SectorId sector, SectorFileType type) override;

    outcome::result<void> moveStorage(SectorId sector,
                                      RegisteredProof seal_proof_type,
                                      SectorFileType types) override;

    outcome::result<FsStat> getFsStat(StorageID id) override;

    outcome::result<std::vector<primitives::StoragePath>> getAccessiblePaths()
        override;

    std::shared_ptr<SectorIndex> getSectorIndex() const override;

    std::shared_ptr<LocalStorage> getLocalStorage() const override;

   private:
    LocalStoreImpl(std::shared_ptr<LocalStorage> storage,
                   std::shared_ptr<SectorIndex> index,
                   gsl::span<const std::string> urls);

    std::shared_ptr<LocalStorage> storage_;
    std::shared_ptr<SectorIndex> index_;
    std::vector<std::string> urls_;
    std::unordered_map<StorageID, std::string> paths_;
    fc::common::Logger logger_;

    mutable std::shared_mutex mutex_;
  };

}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_LOCAL_STORE_HPP
