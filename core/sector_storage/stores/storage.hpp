/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/optional.hpp>

#include "common/outcome.hpp"
#include "primitives/types.hpp"

namespace fc::sector_storage::stores {
  using primitives::FsStat;

  const std::string kStorageConfig = "storage.json";

  struct LocalPath {
    std::string path;
  };

  inline bool operator==(const LocalPath &path1, const LocalPath &path2) {
    return path1.path == path2.path;
  }

  // .storage/storage.json
  struct StorageConfig {
    std::vector<LocalPath> storage_paths;
  };

  inline bool operator==(const StorageConfig &lhs, const StorageConfig &rhs) {
    return lhs.storage_paths == rhs.storage_paths;
  }

  class LocalStorage {
   public:
    virtual ~LocalStorage() = default;

    virtual outcome::result<FsStat> getStat(const std::string &path) const = 0;

    virtual outcome::result<boost::optional<StorageConfig>> getStorage()
        const = 0;

    virtual outcome::result<void> setStorage(
        std::function<void(StorageConfig &)> action) = 0;

    // when file doesn't exist should return StorageError::kFileNotExist
    virtual outcome::result<uint64_t> getDiskUsage(
        const std::string &path) const = 0;
  };
}  // namespace fc::sector_storage::stores
