/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/stores/store.hpp"

#include <boost/asio/io_context.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <shared_mutex>
#include "common/logger.hpp"
#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage::stores {
  using libp2p::basic::Scheduler;

  class LocalStoreImpl : public LocalStore {
   public:
    static outcome::result<std::shared_ptr<LocalStore>> newLocalStore(
        const std::shared_ptr<LocalStorage> &storage,
        const std::shared_ptr<SectorIndex> &index,
        gsl::span<const std::string> urls,
        std::shared_ptr<Scheduler> scheduler);

    outcome::result<void> openPath(const std::string &path) override;

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

    outcome::result<std::vector<primitives::StoragePath>> getAccessiblePaths()
        override;

    std::shared_ptr<SectorIndex> getSectorIndex() const override;

    std::shared_ptr<LocalStorage> getLocalStorage() const override;

    outcome::result<std::function<void()>> reserve(SectorRef sector,
                                                   SectorFileType file_type,
                                                   const SectorPaths &storages,
                                                   PathType path_type) override;

   private:
    LocalStoreImpl(std::shared_ptr<LocalStorage> storage,
                   std::shared_ptr<SectorIndex> index,
                   gsl::span<const std::string> urls);

    outcome::result<void> removeSector(SectorId sector,
                                       SectorFileType type,
                                       const StorageID &storage);
    void reportHealth();

    struct Path {
      explicit Path(std::string path);

      [[nodiscard]] outcome::result<FsStat> getStat(
          const std::shared_ptr<LocalStorage> &local_storage,
          const common::Logger &logger) const;

      std::string local_path;
      int64_t reserved = 0;
      std::map<SectorId, SectorFileType> reservations = {};
    };

    std::shared_ptr<LocalStorage> storage_;
    std::shared_ptr<SectorIndex> index_;
    std::vector<std::string> urls_;
    std::unordered_map<StorageID, std::shared_ptr<Path>> paths_;
    fc::common::Logger logger_;
    Scheduler::Handle handler_;
    std::chrono::milliseconds heartbeat_interval_;
    mutable std::shared_mutex mutex_;
  };

}  // namespace fc::sector_storage::stores
