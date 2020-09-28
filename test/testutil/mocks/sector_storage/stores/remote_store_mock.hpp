/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_REMOTE_STORE_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_REMOTE_STORE_MOCK_HPP

#include <gmock/gmock.h>

#include "sector_storage/stores/store.hpp"

namespace fc::sector_storage::stores {
  class RemoteStoreMock : public RemoteStore {
   public:
    MOCK_METHOD6(acquireSector,
                 outcome::result<AcquireSectorResponse>(SectorId,
                                                        RegisteredProof,
                                                        SectorFileType,
                                                        SectorFileType,
                                                        PathType path_type,
                                                        AcquireMode mode));

    MOCK_METHOD2(remove, outcome::result<void>(SectorId, SectorFileType));

    MOCK_METHOD2(removeCopies, outcome::result<void>(SectorId, SectorFileType));

    MOCK_METHOD3(moveStorage,
                 outcome::result<void>(SectorId,
                                       RegisteredProof,
                                       SectorFileType));

    MOCK_METHOD1(getFsStat, outcome::result<FsStat>(StorageID));

    MOCK_CONST_METHOD0(getSectorIndex, std::shared_ptr<SectorIndex>());

    MOCK_CONST_METHOD0(getLocalStore, std::shared_ptr<LocalStore>());
  };
}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_REMOTE_STORE_MOCK_HPP
