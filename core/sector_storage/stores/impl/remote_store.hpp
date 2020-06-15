/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_REMOTE_STORE_HPP
#define CPP_FILECOIN_REMOTE_STORE_HPP

#include "sector_storage/stores/store.hpp"

#include "sector_storage/stores/impl/local_store.hpp"

namespace fc::sector_storage::stores {

  class RemoteStore : public Store {
   public:
    RemoteStore(std::shared_ptr<LocalStore> local,
                std::shared_ptr<SectorIndex> index);

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
    struct RemoveAcquireSectorResponse {
      std::string path;
      StorageID storage_id;
      std::string url;
    };

    outcome::result<RemoveAcquireSectorResponse> acquireFromRemote(
        SectorId sector,
        RegisteredProof seal_proof_type,
        SectorFileType file_type,
        bool can_seal);

    outcome::result<void> fetch(const std::string &url,
                                const std::string &output_name);

    outcome::result<void> deleteFromRemote(const std::string &url);

    std::shared_ptr<LocalStore> local_;
    std::shared_ptr<SectorIndex> index_;

    mutable std::shared_mutex mutex_;
  };

}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_REMOTE_STORE_HPP
