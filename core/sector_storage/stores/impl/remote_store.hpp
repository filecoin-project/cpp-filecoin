/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/stores/store.hpp"

#include "sector_storage/stores/impl/local_store.hpp"
#include <condition_variable>

namespace fc::sector_storage::stores {

  using HeaderName = std::string;
  using HeaderValue = std::string;

  class RemoteStoreImpl : public RemoteStore {
   public:
    RemoteStoreImpl(std::shared_ptr<LocalStore> local,
                    std::unordered_map<HeaderName, HeaderValue> auth_headers);

    outcome::result<AcquireSectorResponse> acquireSector(
            SectorRef sector,
        SectorFileType existing,
        SectorFileType allocate,
        PathType path_type,
        AcquireMode mode) override;

    outcome::result<void> remove(SectorId sector, SectorFileType type) override;

    outcome::result<void> removeCopies(SectorId sector,
                                       SectorFileType type) override;

    outcome::result<void> moveStorage(SectorRef sector,
                                      SectorFileType types) override;

    outcome::result<FsStat> getFsStat(StorageID id) override;

    std::shared_ptr<SectorIndex> getSectorIndex() const override;

    std::shared_ptr<LocalStore> getLocalStore() const override;

   private:
    outcome::result<std::string> acquireFromRemote(SectorId sector,
                                                   SectorFileType file_type,
                                                   const std::string &dest);

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
