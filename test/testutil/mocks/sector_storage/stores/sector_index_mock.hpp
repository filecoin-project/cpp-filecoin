/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage::stores {
  class SectorIndexMock : public SectorIndex {
   public:
    MOCK_METHOD2(storageAttach,
                 outcome::result<void>(const SectorStorageInfo &storage_info,
                                       const FsStat &stat));
    MOCK_CONST_METHOD1(
        getStorageInfo,
        outcome::result<SectorStorageInfo>(const StorageID &storage_id));
    MOCK_METHOD2(storageReportHealth,
                 outcome::result<void>(const StorageID &storage_id,
                                       const HealthReport &report));
    MOCK_METHOD4(storageDeclareSector,
                 outcome::result<void>(const StorageID &storage_id,
                                       const SectorId &sector,
                                       const SectorFileType &file_type,
                                       bool primary));

    MOCK_METHOD3(storageDropSector,
                 outcome::result<void>(const StorageID &storage_id,
                                       const SectorId &sector,
                                       const SectorFileType &file_type));

    MOCK_METHOD3(
        storageFindSector,
        outcome::result<std::vector<SectorStorageInfo>>(const SectorId &,
                                                  const SectorFileType &,
                                                  boost::optional<SectorSize>));

    MOCK_METHOD3(storageBestAlloc,
                 outcome::result<std::vector<SectorStorageInfo>>(
                     const SectorFileType &, SectorSize, bool));

    MOCK_METHOD3(storageLock,
                 outcome::result<std::shared_ptr<WLock>>(const SectorId &,
                                                         SectorFileType,
                                                         SectorFileType));

    MOCK_METHOD3(storageTryLock,
                 std::shared_ptr<WLock>(const SectorId &,
                                        SectorFileType,
                                        SectorFileType));
  };
}  // namespace fc::sector_storage::stores
