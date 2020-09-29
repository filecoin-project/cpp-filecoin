/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_IMPL_FILESYSTEM_REPOSITORY_HPP
#define FILECOIN_CORE_STORAGE_IMPL_FILESYSTEM_REPOSITORY_HPP

#include <boost/filesystem.hpp>
#include <iostream>

#include "fslock/fslock.hpp"
#include "storage/filestore/path.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/repository/repository.hpp"

namespace fc::storage::repository {

  using filestore::Path;
  using sector_storage::stores::StorageConfig;
  using Version = Repository::Version;
  using primitives::FsStat;

  /**
   * @brief FileSystem implementation of Repository.
   *
   *  Directory structure of Filesystem Repository is:
   * .ipfs/
   * ├── api             <--- running daemon api addr
   * ├── blocks/         <--- objects stored directly on disk
   * │   └── aa          <--- prefix namespacing like git
   * │       └── aa      <--- N tiers
   * ├── config          <--- config file (json or toml)
   * ├── hooks/          <--- hook scripts (not implemented yet)
   * ├── keys/           <--- cryptographic keys
   * │   ├── id.pri      <--- identity private key
   * │   └── id.pub      <--- identity public key
   * ├── datastore/      <--- datastore
   * ├── logs/           <--- 1 or more files (log rotate)
   * │   └── events.log  <--- can be tailed
   * ├── repo.lock       <--- mutex for repo
   * └── version         <--- version file
   *
   */
  class FileSystemRepository : public Repository {
   public:
    inline static const std::string kApiFilename = "api";
    inline static const std::string kConfigFilename = "config.json";
    inline static const std::string kKeysDirectory = "keys";
    inline static const std::string kDatastore = "datastore";
    inline static const std::string kRepositoryLock = "repo.lock";
    inline static const std::string kVersionFilename = "version";
    inline static const std::string kStorageConfig = "storage.json";
    inline static const Version kFileSystemRepositoryVersion = 1;

    FileSystemRepository(std::shared_ptr<IpfsDatastore> ipld_store,
                         std::shared_ptr<KeyStore> keystore,
                         std::shared_ptr<Config> config,
                         std::string repository_path,
                         std::unique_ptr<fslock::Locker> fs_locker);

    static outcome::result<std::shared_ptr<Repository>> create(
        const Path &repo_path,
        const std::string &api_address,
        const leveldb::Options &leveldb_options);

    outcome::result<Version> getVersion() const override;
    outcome::result<StorageConfig> getStorage() override;
    outcome::result<void> setStorage(
        std::function<void(StorageConfig &)> action) override;
    outcome::result<FsStat> getStat(const std::string &path) override;
    outcome::result<int64_t> getDiskUsage(const std::string &path) override;

   private:
    std::mutex storage_mutex_;
    Path repository_path_;
    std::unique_ptr<fslock::Locker> fs_locker_;
    inline static common::Logger logger_ = common::createLogger("repository");
    outcome::result<StorageConfig> nonBlockGetStorage();
  };

}  // namespace fc::storage::repository

#endif  // FILECOIN_CORE_STORAGE_IMPL_FILESYSTEM_REPOSITORY_HPP
