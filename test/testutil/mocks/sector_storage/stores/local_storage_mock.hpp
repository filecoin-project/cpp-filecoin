/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "sector_storage/stores/impl/local_store.hpp"

namespace fc::sector_storage::stores {
  class LocalStorageMock : public LocalStorage {
   public:
    MOCK_CONST_METHOD1(getStat,
                       outcome::result<FsStat>(const std::string &path));

    MOCK_CONST_METHOD0(getStorage,
                       outcome::result<boost::optional<StorageConfig>>());

    MOCK_METHOD1(setStorage,
                 outcome::result<void>(std::function<void(StorageConfig &)>));
    MOCK_CONST_METHOD1(getDiskUsage,
                       outcome::result<uint64_t>(const std::string &path));
  };
}  // namespace fc::sector_storage::stores
