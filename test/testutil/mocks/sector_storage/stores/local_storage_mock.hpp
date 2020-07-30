/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_LOCAL_STORAGE_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_LOCAL_STORAGE_MOCK_HPP

#include <gmock/gmock.h>

#include "sector_storage/stores/impl/local_store.hpp"

namespace fc::sector_storage::stores {
  class LocalStorageMock : public LocalStorage {
   public:
    MOCK_METHOD1(getStat, outcome::result<FsStat>(const std::string &path));

    MOCK_METHOD0(getStorage, outcome::result<StorageConfig>());

    MOCK_METHOD1(setStorage,
                 outcome::result<void>(std::function<void(StorageConfig &)>));
  };
}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_STORES_LOCAL_STORAGE_MOCK_HPP
