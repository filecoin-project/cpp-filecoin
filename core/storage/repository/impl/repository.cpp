/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/repository.hpp"
#include "api/rpc/json.hpp"
#include "storage/repository/repository_error.hpp"
#include "sector_storage/stores/storage_error.hpp"

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#if __APPLE__
#include <sys/mount.h>
#include <sys/param.h>
#elif __linux__
#include <sys/statfs.h>
#endif
using fc::primitives::FsStat;
using fc::sector_storage::stores::StorageConfig;
using fc::storage::config::Config;
using fc::storage::ipfs::IpfsDatastore;
using fc::storage::keystore::KeyStore;
using fc::storage::repository::Repository;
using fc::sector_storage::stores::StorageError;

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
  std::ifstream file{(path).string(), std::ios::binary | std::ios::ate};
  if (!file.good()) {
    return RepositoryError::kInvalidStorageConfig;
  }
  fc::common::Buffer buffer;
  buffer.resize(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(fc::common::span::string(buffer).data(), buffer.size());

  rapidjson::Document j_file;
  j_file.Parse(fc::common::span::cstring(buffer).data(), buffer.size());
  buffer.clear();
  if (j_file.HasParseError()) {
    return RepositoryError::kInvalidStorageConfig;
  }
  return api::decode<StorageConfig>(j_file);
}

fc::outcome::result<void> Repository::writeStorage(
    const boost::filesystem::path &path, StorageConfig config) {
  std::ofstream file{(path).string()};
  if (!file.good()) {
    return RepositoryError::kInvalidStorageConfig;
  }
  auto doc = api::encode<StorageConfig>(config);
  if (doc.HasParseError()) {
    return RepositoryError::kParseJsonError;
  }

  rapidjson::OStreamWrapper osw{file};
  rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};
  if (!doc.Accept(writer)) {
    return RepositoryError::kWriteJsonError;
  }
  return outcome::success();
}

fc::outcome::result<FsStat> Repository::statFs(const std::string &path) {
#if __APPLE__
  struct statfs64 stat;
  if (statfs64(path.c_str(), &stat) != 0) {
    return RepositoryError::kFilesystemStatError;
  }
  return FsStat{.capacity = stat.f_blocks * stat.f_bsize,
                .available = stat.f_bavail * stat.f_bsize,
                .reserved = 0};
#elif __linux__
  struct statfs stat;
  if (statfs(path.c_str(), &stat) != 0) {
    return RepositoryError::kFilesystemStatError;
  }
  return FsStat{.capacity = stat.f_blocks * stat.f_bsize,
                .available = stat.f_bavail * stat.f_bsize,
                .reserved = 0};
#endif
}

fc::outcome::result<uint64_t> Repository::fileSize(const std::string &path) {
  if (!boost::filesystem::exists(path)) {
    return StorageError::kFileNotExist;
  }
#if __APPLE__
  struct statfs64 stat;
  if (statfs64(path.c_str(), &stat) != 0) {
    return RepositoryError::kFilesystemStatError;
  }
  return stat.f_blocks * 512;
#elif __linux__
  struct statfs stat;
  if (statfs(path.c_str(), &stat) != 0) {
    return RepositoryError::kFilesystemStatError;
  }
  return stat.f_blocks * 512;
#endif
}