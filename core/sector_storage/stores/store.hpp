/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <primitives/sector/sector.hpp>
#include "common/enum.hpp"
#include "common/outcome.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/sector_file/sector_file.hpp"
#include "primitives/types.hpp"
#include "sector_storage/stores/index.hpp"
#include "sector_storage/stores/storage.hpp"

namespace fc::sector_storage::stores {
  using primitives::FsStat;
  using primitives::StorageID;
  using primitives::sector::SectorRef;
  using primitives::sector_file::SectorFileType;
  using primitives::sector_file::SectorPaths;

  const std::string kMetaFileName = "sectorstore.json";

  struct AcquireSectorResponse {
    SectorPaths paths;
    SectorPaths storages;
  };

  enum class PathType : int64_t {
    kStorage = 0,
    kSealing,
  };

  inline auto &class_conversion_table(PathType &&) {
    using E = PathType;
    static fc::common::ConversionTable<E, 2> table{
        {{E::kSealing, "sealing"}, {E::kStorage, "storage"}}};
    return table;
  }

  enum class AcquireMode : int64_t {
    kMove = 0,
    kCopy,
  };

  inline auto &class_conversion_table(AcquireMode &&) {
    static fc::common::ConversionTable<AcquireMode, 2> table{
        {{AcquireMode::kMove, "move"}, {AcquireMode::kCopy, "copy"}}};
    return table;
  }

  class Store {
   public:
    virtual ~Store() = default;

    virtual outcome::result<AcquireSectorResponse> acquireSector(
        SectorRef sector,
        SectorFileType existing,
        SectorFileType allocate,
        PathType path_type,
        AcquireMode mode) = 0;

    virtual outcome::result<void> remove(SectorId sector,
                                         SectorFileType type) = 0;

    /**
     * @note like remove, but doesn't remove the primary sector copy, nor the
     * last non-primary copy if there no primary copies
     */
    virtual outcome::result<void> removeCopies(SectorId sector,
                                               SectorFileType type) = 0;

    virtual outcome::result<void> moveStorage(SectorRef sector,
                                              SectorFileType types) = 0;

    virtual outcome::result<FsStat> getFsStat(StorageID id) = 0;

    virtual std::shared_ptr<SectorIndex> getSectorIndex() const = 0;
  };

  class LocalStore : public Store {
   public:
    virtual outcome::result<void> openPath(const std::string &path) = 0;

    virtual outcome::result<std::vector<primitives::StoragePath>>
    getAccessiblePaths() = 0;

    virtual std::shared_ptr<LocalStorage> getLocalStorage() const = 0;

    virtual outcome::result<std::function<void()>> reserve(
        SectorRef sector,
        SectorFileType file_type,
        const SectorPaths &storages,
        PathType path_type) = 0;
  };

  class RemoteStore : public Store {
   public:
    virtual std::shared_ptr<LocalStore> getLocalStore() const = 0;
  };

}  // namespace fc::sector_storage::stores
