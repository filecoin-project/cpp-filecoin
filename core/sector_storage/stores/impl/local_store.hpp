/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_LOCAL_STORE_HPP
#define CPP_FILECOIN_LOCAL_STORE_HPP

#include "sector_storage/stores/store.hpp"

#include <shared_mutex>
#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage::stores {

  class LocalStorage {
   public:
    virtual ~LocalStorage() = default;

    virtual outcome::result<FsStat> getStat(const std::string &path) = 0;

    virtual outcome::result<std::vector<std::string>> getPaths() = 0;
  };

  const std::string kMetaFileName = "sectorstore.json";

  class LocalStore : public Store {
   public:
    static outcome::result<std::shared_ptr<LocalStore>> newLocalStore(
        std::shared_ptr<LocalStorage> storage,
        std::shared_ptr<SectorIndex> index,
        gsl::span<std::string> urls);

    outcome::result<void> openPath(const std::string &path);

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

   private:
    LocalStore(std::shared_ptr<LocalStorage> storage,
               std::shared_ptr<SectorIndex> index,
               gsl::span<std::string> urls);

    std::shared_ptr<LocalStorage> storage_;
    std::shared_ptr<SectorIndex> index_;
    std::vector<std::string> urls_;
    std::unordered_map<StorageID, std::string> paths_;

    mutable std::shared_mutex mutex_;
  };

};  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_LOCAL_STORE_HPP
