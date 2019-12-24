/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_IMPL_FILESYSTEM_REPOSITORY_HPP
#define FILECOIN_CORE_STORAGE_IMPL_FILESYSTEM_REPOSITORY_HPP

#include <iostream>

#include "fslock/fslock.hpp"
#include "storage/filestore/path.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/repository/repository.hpp"

namespace fc::storage::repository {

  using filestore::Path;
  using Version = Repository::Version;

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
    static constexpr char kApiFilename[] = "api";
    static constexpr char kConfigFilename[] = "config.json";
    static constexpr char kKeysDirectory[] = "keys";
    static constexpr char kDatastore[] = "datastore";
    static constexpr char kRepositoryLock[] = "repo.lock";
    static constexpr char kVersionFilename[] = "version";
    static constexpr Version kFileSystemRepositoryVersion = 1;

    FileSystemRepository(std::shared_ptr<IpfsDatastore> ipld_store,
                         std::shared_ptr<KeyStore> keystore,
                         std::shared_ptr<Config> config,
                         std::string repository_path,
                         std::unique_ptr<fslock::Locker> fs_locker);

    ~FileSystemRepository() override = default;

    static outcome::result<std::shared_ptr<Repository>> create(
        const Path &repo_path,
        const std::string &api_address,
        const leveldb::Options &leveldb_options);

    outcome::result<Version> getVersion() const override;

   private:
    Path repository_path_;
    std::unique_ptr<fslock::Locker> fs_locker_;
  };

}  // namespace fc::storage::repository

#endif  // FILECOIN_CORE_STORAGE_IMPL_FILESYSTEM_REPOSITORY_HPP
