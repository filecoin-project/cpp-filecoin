/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <api/rpc/json.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sector_storage/stores/store.hpp>

#include "storage/repository/impl/in_memory_repository.hpp"

#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "primitives/types.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "storage/repository/impl/filesystem_repository.hpp"
#include "storage/repository/repository_error.hpp"

using fc::crypto::bls::BlsProviderImpl;
using fc::crypto::secp256k1::Secp256k1Sha256ProviderImpl;
using fc::primitives::LocalStorageMeta;
using fc::sector_storage::stores::kMetaFileName;
using fc::sector_storage::stores::LocalPath;
using fc::sector_storage::stores::StorageConfig;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::keystore::InMemoryKeyStore;
using fc::storage::repository::InMemoryRepository;
using fc::storage::repository::Repository;

InMemoryRepository::InMemoryRepository()
    : Repository(std::make_shared<InMemoryDatastore>(),
                 std::make_shared<InMemoryKeyStore>(
                     std::make_shared<BlsProviderImpl>(),
                     std::make_shared<Secp256k1Sha256ProviderImpl>()),
                 std::make_shared<Config>()) {}

fc::outcome::result<std::shared_ptr<Repository>> InMemoryRepository::create(
    const std::string &config_path) {
  auto res = std::make_shared<InMemoryRepository>();
  OUTCOME_TRY(res->loadConfig(config_path));
  return res;
}

fc::outcome::result<unsigned int> InMemoryRepository::getVersion() const {
  return kInMemoryRepositoryVersion;
}

fc::outcome::result<fc::primitives::FsStat> InMemoryRepository::getStat(
    const std::string &path) {
  return statFs(path);
}

fc::outcome::result<StorageConfig> InMemoryRepository::getStorage() {
  const std::lock_guard<std::mutex> lock(storage_mutex_);
  return nonBlockGetStorage();
}

fc::outcome::result<StorageConfig> InMemoryRepository::nonBlockGetStorage() {
  if (storageConfig_.storage_paths.empty()) {
    OUTCOME_TRY(storage_path, path());
    storageConfig_ =
        StorageConfig{std::vector<LocalPath>{{storage_path.string()}}};
  }
  return storageConfig_;
}

fc::outcome::result<boost::filesystem::path> InMemoryRepository::path() {
  if (!tempDir.empty()) {
    return tempDir;
  }
  boost::filesystem::path tempPath = boost::filesystem::unique_path();
  if (!boost::filesystem::create_directory(tempPath)) {
    return RepositoryError::kTempDirectoryCreationError;
  }
  boost::filesystem::path filePath = tempPath;
  filePath /= FileSystemRepository::kStorageConfig;
  if (true) {  // check if repository type is StorageMiner
    OUTCOME_TRY(writeStorage(
        filePath, StorageConfig{std::vector<LocalPath>{{tempPath.string()}}}));
  }
  LocalStorageMeta meta_storage{
      .id = boost::uuids::to_string(boost::uuids::random_generator()()),
      .weight = 10,
      .can_seal = true,
      .can_store = true};
  boost::filesystem::path sector_path = tempPath;
  sector_path /= kMetaFileName;
  std::ofstream file{(sector_path).string()};
  if (!file.good()) {
    return RepositoryError::kInvalidStorageConfig;
  }
  auto doc = api::encode<LocalStorageMeta>(meta_storage);
  if (doc.HasParseError()) {
    return RepositoryError::kParseJsonError;
  }

  rapidjson::OStreamWrapper osw{file};
  rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};
  if (!doc.Accept(writer)) {
    return RepositoryError::kWriteJsonError;
  }
  tempDir = tempPath;
  return tempPath;
}

fc::outcome::result<void> InMemoryRepository::setStorage(
    std::function<void(StorageConfig &)> action) {
  const std::lock_guard<std::mutex> lock(storage_mutex_);
  OUTCOME_TRY(nonBlockGetStorage());
  action(storageConfig_);
  return fc::outcome::success();
}

fc::outcome::result<int64_t> InMemoryRepository::getDiskUsage(
    const std::string &path) {
  return fc::outcome::success();
}