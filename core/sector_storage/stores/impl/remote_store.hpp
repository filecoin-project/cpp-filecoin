/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_REMOTE_STORE_HPP
#define CPP_FILECOIN_REMOTE_STORE_HPP

#include "sector_storage/stores/store.hpp"

#include "sector_storage/stores/impl/local_store.hpp"

namespace fc::sector_storage::stores {

  using HeaderName = std::string;
  using HeaderValue = std::string;

  class RemoteStoreImpl : public RemoteStore {
   public:
    RemoteStoreImpl(
        std::shared_ptr<LocalStore> local,
        std::unordered_map<HeaderName, HeaderValue> auth_headers);

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

    std::shared_ptr<SectorIndex> getSectorIndex() const override;

    std::shared_ptr<LocalStore> getLocalStore() const override;

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
                                const std::string &output_path);

    outcome::result<void> deleteFromRemote(const std::string &url);

    std::shared_ptr<LocalStore> local_;
    std::shared_ptr<SectorIndex> sector_index_;

    std::unordered_map<HeaderName, HeaderValue> auth_headers_;

    bool unlock_;
    std::condition_variable cv_;
    std::set<SectorId> processing_;
    std::mutex waiting_room_;
    std::mutex mutex_;

    common::Logger logger_;
  };

}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_REMOTE_STORE_HPP
