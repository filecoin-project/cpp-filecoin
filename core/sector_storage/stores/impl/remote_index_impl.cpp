/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/remote_index_impl.hpp"

namespace fc::sector_storage::stores {

  outcome::result<void> stores::RemoteSectorIndexImpl::storageAttach(
      const StorageInfo &storage_info, const FsStat &stat) {
    return api_->StorageAttach(storage_info, stat);
  }

  outcome::result<StorageInfo> stores::RemoteSectorIndexImpl::getStorageInfo(
      const StorageID &storage_id) const {
    return api_->StorageInfo(storage_id);
  }

  outcome::result<void> stores::RemoteSectorIndexImpl::storageReportHealth(
      const StorageID &storage_id, const HealthReport &report) {
    return api_->StorageReportHealth(storage_id, report);
  }

  outcome::result<void> stores::RemoteSectorIndexImpl::storageDeclareSector(
      const StorageID &storage_id,
      const SectorId &sector,
      const SectorFileType &file_type,
      bool primary) {
    return api_->StorageDeclareSector(storage_id, sector, file_type, primary);
  }

  outcome::result<void> stores::RemoteSectorIndexImpl::storageDropSector(
      const StorageID &storage_id,
      const SectorId &sector,
      const SectorFileType &file_type) {
    return api_->StorageDropSector(storage_id, sector, file_type);
  }

  outcome::result<std::vector<SectorStorageInfo>>
  stores::RemoteSectorIndexImpl::storageFindSector(
      const SectorId &sector,
      const SectorFileType &file_type,
      boost::optional<SectorSize> fetch_sector_size) {
    return api_->StorageFindSector(sector, file_type, fetch_sector_size);
  }

  outcome::result<std::vector<StorageInfo>>
  stores::RemoteSectorIndexImpl::storageBestAlloc(
      const SectorFileType &allocate,
      SectorSize sector_size,
      bool sealing_mode) {
    return api_->StorageBestAlloc(allocate, sector_size, sealing_mode);
  }

  outcome::result<std::shared_ptr<WLock>>
  stores::RemoteSectorIndexImpl::storageLock(const SectorId &sector,
                                             SectorFileType read,
                                             SectorFileType write) {
    return IndexErrors::kNotSupportedMethod;
  }

  std::shared_ptr<WLock> stores::RemoteSectorIndexImpl::storageTryLock(
      const SectorId &sector, SectorFileType read, SectorFileType write) {
    return nullptr;  // kNotSupported
  }

  RemoteSectorIndexImpl::RemoteSectorIndexImpl(
      std::shared_ptr<StorageMinerApi> api)
      : api_{std::move(api)} {}
}  // namespace fc::sector_storage::stores
