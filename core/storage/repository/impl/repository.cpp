/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/repository.hpp"

#include <sys/stat.h>
#if __APPLE__
#include <sys/mount.h>
#include <sys/param.h>

#define STAT64(x) x
#elif __linux__
#include <sys/statfs.h>

#define STAT64(x) x##64
#endif

#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "sector_storage/stores/storage_error.hpp"
#include "storage/repository/repository_error.hpp"

using fc::primitives::FsStat;
using fc::sector_storage::stores::StorageConfig;
using fc::sector_storage::stores::StorageError;
using fc::storage::config::Config;
using fc::storage::ipfs::IpfsDatastore;
using fc::storage::keystore::KeyStore;
using fc::storage::repository::Repository;

Repository::Repository(std::shared_ptr<IpfsDatastore> ipldStore,
                       std::shared_ptr<KeyStore> keystore,
                       std::shared_ptr<Config> config)
    : ipld_store_(std::move(ipldStore)),
      keystore_(std::move(keystore)),
      config_(std::move(config)) {}

std::shared_ptr<IpfsDatastore> Repository::getIpldStore() const noexcept {
  return ipld_store_;
}

std::shared_ptr<KeyStore> Repository::getKeyStore() const noexcept {
  return keystore_;
}

std::shared_ptr<Config> Repository::getConfig() const noexcept {
  return config_;
}

fc::outcome::result<void> Repository::loadConfig(const std::string &filename) {
  return config_->load(filename);
}

fc::outcome::result<StorageConfig> Repository::storageFromFile(
    const boost::filesystem::path &path) {
  OUTCOME_TRY(text, common::readFile(path.string()));
  OUTCOME_TRY(j_file, codec::json::parse(text));
  return api::decode<StorageConfig>(j_file);
}

fc::outcome::result<void> Repository::writeStorage(
    const boost::filesystem::path &path, StorageConfig config) {
  auto doc = api::encode<StorageConfig>(config);
  OUTCOME_TRY(text, codec::json::format(&doc));
  OUTCOME_TRY(common::writeFile(path.string(), text));
  return outcome::success();
}

fc::outcome::result<FsStat> Repository::getStat(const std::string &path) {
  struct STAT64(statfs) stat;
  if (STAT64(statfs)(path.c_str(), &stat) != 0) {
    return RepositoryError::kFilesystemStatError;
  }
  return FsStat{.capacity = stat.f_blocks * stat.f_bsize,
                .available = stat.f_bavail * stat.f_bsize,
                .reserved = 0};
}

fc::outcome::result<uint64_t> Repository::getDiskUsage(
    const std::string &path) {
  if (!boost::filesystem::exists(path)) {
    return StorageError::kFileNotExist;
  }
  struct STAT64(stat) fstat;
  if (STAT64(stat)(path.c_str(), &fstat) != 0) {
    return RepositoryError::kFilesystemStatError;
  }
  return fstat.st_blocks * 512;
}
