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

  outcome::result<std::vector<StorageInfo>>
  stores::RemoteSectorIndexImpl::storageFindSector(
      const SectorId &sector,
      const SectorFileType &file_type,
      boost::optional<RegisteredSealProof> fetch_seal_proof_type) {
    return api_->StorageFindSector(sector, file_type, fetch_seal_proof_type);
  }

  outcome::result<std::vector<StorageInfo>>
  stores::RemoteSectorIndexImpl::storageBestAlloc(
      const SectorFileType &allocate,
      RegisteredSealProof seal_proof_type,
      bool sealing_mode) {
    return api_->StorageBestAlloc(allocate, seal_proof_type, sealing_mode);
  }

  outcome::result<std::unique_ptr<Lock>>
  stores::RemoteSectorIndexImpl::storageLock(const SectorId &sector,
                                             SectorFileType read,
                                             SectorFileType write) {
    return IndexErrors::kNotSupportedMethod;
  }

  std::unique_ptr<Lock> stores::RemoteSectorIndexImpl::storageTryLock(
      const SectorId &sector, SectorFileType read, SectorFileType write) {
    return nullptr;  // kNotSupported
  }

  RemoteSectorIndexImpl::RemoteSectorIndexImpl(
      std::shared_ptr<StorageMinerApi> api)
      : api_{std::move(api)} {}
}  // namespace fc::sector_storage::stores
