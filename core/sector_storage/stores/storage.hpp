/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORES_STORAGE_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORES_STORAGE_HPP

#include "primitives/types.hpp"

namespace fc::sector_storage::stores {
  using primitives::FsStat;

  struct LocalPath {
    std::string path;
  };

  inline bool operator==(const LocalPath& path1, const LocalPath& path2) {
    if(path1.path == path2.path)
      return true;
    else
      return false;
  }

  // .lotusstorage/storage.json
  struct StorageConfig {
    std::vector<LocalPath> storage_paths;
  };

  class LocalStorage {
   public:
    virtual ~LocalStorage() = default;

    virtual outcome::result<FsStat> getStat(const std::string &path) = 0;

    virtual outcome::result<StorageConfig> getStorage() = 0;

    virtual outcome::result<void> setStorage(
        std::function<void(StorageConfig &)> action) = 0;
  };
}  // namespace fc::sector_storage::stores

#endif  // CPP_FILECOIN_CORE_SECTOR_STORES_STORAGE_HPP
