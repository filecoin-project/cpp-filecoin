/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "sector_storage/stores/store.hpp"

namespace fc::sector_storage::stores {
  class LocalStoreMock : public LocalStore {
   public:
    MOCK_METHOD1(openPath, outcome::result<void>(const std::string &));

    MOCK_METHOD0(getAccessiblePaths,
                 outcome::result<std::vector<primitives::StoragePath>>());

    MOCK_METHOD5(acquireSector,
                 outcome::result<AcquireSectorResponse>(SectorRef,
                                                        SectorFileType,
                                                        SectorFileType,
                                                        PathType path_type,
                                                        AcquireMode mode));

    MOCK_METHOD2(remove, outcome::result<void>(SectorId, SectorFileType));

    MOCK_METHOD2(removeCopies, outcome::result<void>(SectorId, SectorFileType));

    MOCK_METHOD2(moveStorage, outcome::result<void>(SectorRef, SectorFileType));

    MOCK_METHOD1(getFsStat, outcome::result<FsStat>(StorageID));

    MOCK_CONST_METHOD0(getLocalStorage, std::shared_ptr<LocalStorage>());

    MOCK_CONST_METHOD0(getSectorIndex, std::shared_ptr<SectorIndex>());

    MOCK_METHOD4(
        reserve,
        outcome::result<std::function<void()>>(SectorRef,
                                               SectorFileType file_type,
                                               const SectorPaths &storages,
                                               PathType path_type));
  };
}  // namespace fc::sector_storage::stores
