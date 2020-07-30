/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_STORES_INDEX_LOCK_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_STORES_INDEX_LOCK_HPP

#include <condition_variable>
#include <mutex>

#include "primitives/sector_file/sector_file.hpp"
#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage::stores {
  using primitives::sector::SectorId;
  using primitives::sector_file::kSectorFileTypeBits;
  using primitives::sector_file::SectorFileType;

  struct IndexLock : public std::enable_shared_from_this<IndexLock> {
    struct Lock : public stores::Lock {
      const SectorId sector;
      const SectorFileType read, write;
      std::shared_ptr<IndexLock> index;

      Lock(const SectorId &sector, SectorFileType read, SectorFileType write)
          : sector{sector}, read{read}, write{write} {
        assert(((read | write) >> kSectorFileTypeBits) == 0);
      }
      Lock(Lock &&) = default;
      ~Lock() override;
    };

    struct Sector {
      bool canLock(SectorFileType read, SectorFileType write) const;

      std::mutex mutex;
      std::condition_variable cv;
      std::array<size_t, kSectorFileTypeBits> read;
      SectorFileType write;
      size_t refs;
    };

    bool lock(Lock &lock, bool wait);
    void unlock(Lock &lock);

    std::mutex mutex;
    std::map<SectorId, Sector> sectors;
  };
}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_STORES_INDEX_LOCK_HPP
