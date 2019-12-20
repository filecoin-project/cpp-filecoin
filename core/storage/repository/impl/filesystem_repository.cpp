/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/impl/filesystem_repository.hpp"

#include <utility>

#include "boost/filesystem.hpp"
#include "crypto/bls_provider/impl/bls_provider_impl.hpp"
#include "libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp"
#include "primitives/address/impl/address_verifier_impl.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/keystore/impl/filesystem/filesystem_keystore.hpp"
#include "storage/repository/repository_error.hpp"

using fc::crypto::bls::impl::BlsProviderImpl;
using fc::primitives::address::AddressVerifierImpl;
using fc::storage::ipfs::LeveldbDatastore;
using fc::storage::keystore::FileSystemKeyStore;
using fc::storage::repository::FileSystemRepository;
using libp2p::crypto::secp256k1::Secp256k1ProviderImpl;
using Version = fc::storage::repository::Repository::Version;
using fc::storage::repository::RepositoryError;

FileSystemRepository::FileSystemRepository(
    std::shared_ptr<IpfsDatastore> ipld_store,
    std::shared_ptr<KeyStore> keystore,
    std::shared_ptr<Config> config,
    std::string repository_path,
    std::unique_ptr<fslock::Locker> fs_locker)
    : Repository{std::move(ipld_store), std::move(keystore), std::move(config)},
      repository_path_{std::move(repository_path)},
      fs_locker_{std::move(fs_locker)} {}

fc::outcome::result<std::shared_ptr<FileSystemRepository>>
FileSystemRepository::create(const Path &repo_path,
                             const std::string &api_address,
                             const leveldb::Options &leveldb_options) {
  // check version if version file exists
  auto version_filename =
      repo_path + fc::storage::filestore::DELIMITER + kVersionFilename;
  if (boost::filesystem::exists(version_filename)) {
    boost::filesystem::ifstream version_is(version_filename);
    std::string version_line;
    std::getline(version_is, version_line);
    // returns version number or 0 in case of failure
    Version version = atoi(version_line.c_str());
    if (version != kFileSystemRepositoryVersion)
      return RepositoryError::WRONG_VERSION;
  }

  // try lock
  auto lock_filename =
      repo_path + fc::storage::filestore::DELIMITER + kRepositoryLock;
  std::unique_ptr<fslock::Locker> fs_locker =
      std::make_unique<fslock::Locker>();
  OUTCOME_TRY(fs_locker->lock(lock_filename));

  // load config if exists
  auto config_filename =
      repo_path + fc::storage::filestore::DELIMITER + kConfigFilename;
  auto config = std::make_shared<Config>();
  if (boost::filesystem::exists(config_filename)) {
    OUTCOME_TRY(config->load(config_filename));
  }

  // write api address
  auto api_filename =
      repo_path + fc::storage::filestore::DELIMITER + kApiFilename;
  boost::filesystem::ofstream api_os(api_filename);
  api_os << api_address << std::endl;
  api_os.close();

  // write version
  boost::filesystem::ofstream version_os(version_filename);
  version_os << kFileSystemRepositoryVersion << std::endl;
  version_os.close();

  // create datastore
  auto datastore_path =
      repo_path + fc::storage::filestore::DELIMITER + kDatastore;
  OUTCOME_TRY(ipfs_datastore,
              LeveldbDatastore::create(datastore_path, leveldb_options));

  // create keystore
  auto keystore_path =
      repo_path + fc::storage::filestore::DELIMITER + kKeysDirectory;
  boost::filesystem::create_directory(keystore_path);
  auto keystore = std::make_shared<FileSystemKeyStore>(
      keystore_path,
      std::make_shared<BlsProviderImpl>(),
      std::make_shared<Secp256k1ProviderImpl>(),
      std::make_shared<AddressVerifierImpl>());

  return std::make_shared<FileSystemRepository>(
      ipfs_datastore, keystore, config, repo_path, std::move(fs_locker));
}

fc::outcome::result<Version> FileSystemRepository::getVersion() const {
  return kFileSystemRepositoryVersion;
}
