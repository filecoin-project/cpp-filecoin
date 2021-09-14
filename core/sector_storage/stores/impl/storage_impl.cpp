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
      : root_path_{path} {}

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
    const auto &config_path = root_path_ / kStorageConfig;
    if (!boost::filesystem::exists(config_path)) {
      return boost::none;
    }
    OUTCOME_TRY(text, common::readFile(config_path));
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
    OUTCOME_TRY(common::writeFile(root_path_ / kStorageConfig, text));
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

  outcome::result<void> LocalStorageImpl::setApiToken(
      const std::string &token) {
    return common::writeFile(root_path_ / kApiToken,
                             common::span::cbytes(token));
  }

  outcome::result<std::string> LocalStorageImpl::getApiToken() const {
    const auto &token_path = root_path_ / kApiToken;
    if (!boost::filesystem::exists(token_path)) {
      return StorageError::kFileNotExist;
    }
    OUTCOME_TRY(token, common::readFile(token_path));
    return std::string(common::span::cast<char>(token.data()), token.size());
  }

  outcome::result<void> LocalStorageImpl::setSecret(const std::string &secret) {
    return common::writeFile(root_path_ / kApiSecret,
                             common::span::cbytes(secret));
  }

  outcome::result<std::string> LocalStorageImpl::getSecret() const {
    const auto &secret_path = root_path_ / kApiSecret;
    if (!boost::filesystem::exists(secret_path)) {
      return StorageError::kFileNotExist;
    }
    OUTCOME_TRY(secret, common::readFile(secret_path));
    return std::string(common::span::cast<char>(secret.data()), secret.size());
  }
}  // namespace fc::sector_storage::stores
