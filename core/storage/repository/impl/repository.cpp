/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/repository.hpp"
#include "api/rpc/json.hpp"
#include "storage/repository/repository_error.hpp"

#include "codec/json/json.hpp"
#include "common/file.hpp"

using fc::sector_storage::stores::StorageConfig;
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