/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_SECTOR_INDEX_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_SECTOR_INDEX_MOCK_HPP

#include <gmock/gmock.h>

#include "sector_storage/stores/index.hpp"

namespace fc::sector_storage::stores {
  class SectorIndexMock : public SectorIndex {
   public:
    MOCK_METHOD2(storageAttach,
                 outcome::result<void>(const StorageInfo &storage_info,
                                       const FsStat &stat));
    MOCK_CONST_METHOD1(
        getStorageInfo,
        outcome::result<StorageInfo>(const StorageID &storage_id));
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
        outcome::result<std::vector<StorageInfo>>(
            const SectorId &sector,
            const SectorFileType &file_type,
            boost::optional<RegisteredSealProof> fetch_seal_proof_type));

    MOCK_METHOD3(storageBestAlloc,
                 outcome::result<std::vector<StorageInfo>>(
                     const SectorFileType &allocate,
                     RegisteredSealProof seal_proof_type,
                     bool sealing_mode));

    MOCK_METHOD3(storageLock,
                 outcome::result<std::unique_ptr<Lock>>(const SectorId &,
                                                        SectorFileType,
                                                        SectorFileType));

    MOCK_METHOD3(storageTryLock,
                 std::unique_ptr<Lock>(const SectorId &,
                                       SectorFileType,
                                       SectorFileType));
  };
}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_SECTOR_INDEX_MOCK_HPP
