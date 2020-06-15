/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/remote_store.hpp"

#include <utility>

namespace fc::sector_storage::stores {

  RemoteStore::RemoteStore(std::shared_ptr<LocalStore> local,
                           std::shared_ptr<SectorIndex> index)
      : local_(std::move(local)), index_(std::move(index)) {}

  outcome::result<AcquireSectorResponse> RemoteStore::acquireSector(
      SectorId sector,
      RegisteredProof seal_proof_type,
      SectorFileType existing,
      SectorFileType allocate,
      bool can_seal) {
    return outcome::success();
  }

  outcome::result<void> RemoteStore::remove(SectorId sector,
                                            SectorFileType type) {
    return outcome::success();
  }

  outcome::result<void> RemoteStore::moveStorage(
      SectorId sector, RegisteredProof seal_proof_type, SectorFileType types) {
    return outcome::success();
  }

  outcome::result<FsStat> RemoteStore::getFsStat(StorageID id) {
    return outcome::success();
  }

  outcome::result<RemoteStore::RemoveAcquireSectorResponse>
  RemoteStore::acquireFromRemote(SectorId sector,
                                 RegisteredProof seal_proof_type,
                                 SectorFileType file_type,
                                 bool can_seal) {
    return outcome::success();
  }

  outcome::result<void> RemoteStore::fetch(const std::string &url,
                                           const std::string &output_name) {
    return outcome::success();
  }

  outcome::result<void> RemoteStore::deleteFromRemote(const std::string &url) {
    return outcome::success();
  }

}  // namespace fc::sector_storage::stores
