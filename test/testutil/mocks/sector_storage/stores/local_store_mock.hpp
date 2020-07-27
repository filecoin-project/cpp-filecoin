/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_LOCAL_STORE_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_LOCAL_STORE_MOCK_HPP

#include <gmock/gmock.h>

#include "sector_storage/stores/store.hpp"

namespace fc::sector_storage::stores {
  class LocalStoreMock : public LocalStore {
   public:
    MOCK_METHOD1(openPath, outcome::result<void>(const std::string &));

    MOCK_METHOD0(getAccessiblePaths,
                 outcome::result<std::vector<primitives::StoragePath>>());

    MOCK_METHOD5(acquireSector,
                 outcome::result<AcquireSectorResponse>(SectorId,
                                                        RegisteredProof,
                                                        SectorFileType,
                                                        SectorFileType,
                                                        bool));

    MOCK_METHOD2(remove, outcome::result<void>(SectorId, SectorFileType));

    MOCK_METHOD3(moveStorage,
                 outcome::result<void>(SectorId,
                                       RegisteredProof,
                                       SectorFileType));

    MOCK_METHOD1(getFsStat, outcome::result<FsStat>(StorageID));

    MOCK_CONST_METHOD0(getLocalStorage, std::shared_ptr<LocalStorage>());

    MOCK_CONST_METHOD0(getSectorIndex, std::shared_ptr<SectorIndex>());
  };
}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_LOCAL_STORE_MOCK_HPP
