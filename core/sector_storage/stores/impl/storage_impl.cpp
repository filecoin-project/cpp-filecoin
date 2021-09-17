/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/impl/storage_impl.hpp"

#include <boost/filesystem.hpp>
#include <utility>

#include <sys/stat.h>
#if __APPLE__
#include <sys/mount.h>
#include <sys/param.h>

#define STAT64(x) x
#elif __linux__
#include <sys/statfs.h>

#define STAT64(x) x##64
#endif

#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "sector_storage/stores/storage_error.hpp"

// maybe manual
#include "api/rpc/json.hpp"

namespace fc::sector_storage::stores {
  LocalStorageImpl::LocalStorageImpl(const boost::filesystem::path &path)
      : config_path_{boost::filesystem::path(path) / kStorageConfig} {}

  outcome::result<FsStat> LocalStorageImpl::getStat(
      const std::string &path) const {
    struct STAT64(statfs) stat;
    if (STAT64(statfs)(path.c_str(), &stat) != 0) {
      return StorageError::kFilesystemStatError;
    }
    return FsStat{.capacity = stat.f_blocks * stat.f_bsize,
                  .available = stat.f_bavail * stat.f_bsize,
                  .reserved = 0};
  }

  outcome::result<boost::optional<StorageConfig>> LocalStorageImpl::getStorage()
      const {
    if (!boost::filesystem::exists(config_path_)) {
      return boost::none;
    }
    OUTCOME_TRY(text, common::readFile(config_path_));
    OUTCOME_TRY(j_file, codec::json::parse(text));
    OUTCOME_TRY(decoded, api::decode<StorageConfig>(j_file));
    return std::move(decoded);
  }

  outcome::result<void> LocalStorageImpl::setStorage(
      std::function<void(StorageConfig &)> action) {
    OUTCOME_TRY(config, getStorage());
    if (!config.has_value()) {
      config = StorageConfig{};
    }
    action(config.value());
    OUTCOME_TRY(text, codec::json::format(api::encode(config)));
    OUTCOME_TRY(common::writeFile(config_path_, text));
    return outcome::success();
  }

  outcome::result<uint64_t> LocalStorageImpl::getDiskUsage(
      const std::string &path) const {
    if (!boost::filesystem::exists(path)) {
      return StorageError::kFileNotExist;
    }
    struct STAT64(stat) fstat;
    if (STAT64(stat)(path.c_str(), &fstat) != 0) {
      return StorageError::kFilesystemStatError;
    }
    return fstat.st_blocks * 512;
  }
}  // namespace fc::sector_storage::stores
