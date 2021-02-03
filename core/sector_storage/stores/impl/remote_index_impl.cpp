/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/remote_index_impl.hpp"

namespace fc::sector_storage::stores {

  outcome::result<void> stores::RemoteSectorIndexImpl::storageAttach(
      const StorageInfo &storage_info, const FsStat &stat) {
    return outcome::success();
  }

  outcome::result<StorageInfo> stores::RemoteSectorIndexImpl::getStorageInfo(
      const StorageID &storage_id) const {
    return outcome::success();
  }

  outcome::result<void> stores::RemoteSectorIndexImpl::storageReportHealth(
      const StorageID &storage_id, const HealthReport &report) {
    return outcome::success();
  }

  outcome::result<void> stores::RemoteSectorIndexImpl::storageDeclareSector(
      const StorageID &storage_id,
      const SectorId &sector,
      const SectorFileType &file_type,
      bool primary) {
    return outcome::success();
  }

  outcome::result<void> stores::RemoteSectorIndexImpl::storageDropSector(
      const StorageID &storage_id,
      const SectorId &sector,
      const SectorFileType &file_type) {
    return outcome::success();
  }

  outcome::result<std::vector<StorageInfo>>
  stores::RemoteSectorIndexImpl::storageFindSector(
      const SectorId &sector,
      const SectorFileType &file_type,
      boost::optional<RegisteredSealProof> fetch_seal_proof_type) {
    return outcome::success();
  }

  outcome::result<std::vector<StorageInfo>>
  stores::RemoteSectorIndexImpl::storageBestAlloc(
      const SectorFileType &allocate,
      RegisteredSealProof seal_proof_type,
      bool sealing_mode) {
    return outcome::success();
  }

  outcome::result<std::unique_ptr<Lock>>
  stores::RemoteSectorIndexImpl::storageLock(const SectorId &sector,
                                             SectorFileType read,
                                             SectorFileType write) {
    // TODO: Possible problem with multimachine
    return outcome::success();
  }

  std::unique_ptr<Lock> stores::RemoteSectorIndexImpl::storageTryLock(
      const SectorId &sector, SectorFileType read, SectorFileType write) {
    // TODO: Possible problem with multimachine
    return nullptr;
  }
}  // namespace fc::sector_storage::stores
