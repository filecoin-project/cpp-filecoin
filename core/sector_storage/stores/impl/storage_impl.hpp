/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/stores/storage.hpp"

namespace fc::sector_storage::stores {
  struct LocalStorageImpl : LocalStorage {
    LocalStorageImpl(std::string path);

    outcome::result<FsStat> getStat(const std::string &path) override;
    outcome::result<StorageConfig> getStorage() override;
    outcome::result<void> setStorage(
        std::function<void(StorageConfig &)> action) override;
    outcome::result<uint64_t> getDiskUsage(const std::string &path) override;

    std::string config_path;
  };
}  // namespace fc::sector_storage::stores
