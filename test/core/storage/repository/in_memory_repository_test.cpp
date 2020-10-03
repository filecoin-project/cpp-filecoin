/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/impl/in_memory_repository.hpp"

#include "storage/repository/repository_error.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::sector_storage::stores::LocalPath;
using fc::sector_storage::stores::StorageConfig;
using fc::storage::repository::InMemoryRepository;
using test::BaseFS_Test;

/**
 * @given Repository with a .json file with a config
 * @when Parse .json file and create a temp directory and a LocalStorageMeta
 * object and then write parsed Storage config file and LocalStorageMeta to the
 * temp dir
 * @then Storage config with a path of the temp directory
 */
TEST(InMemoryRepositoryTest, GetStorage) {
  InMemoryRepository repository;
  EXPECT_OUTCOME_TRUE(config, repository.getStorage());
  EXPECT_OUTCOME_TRUE(path, repository.path())
  std::vector<LocalPath> paths = {{path.string()}};
  ASSERT_EQ(config.storage_paths, paths);
}

/**
 * @given Repository with a .json config file
 * @when Get a Storage config from a parsed json and apply some function to the
 * storage config
 * @then Storage config with an update paths
 */
TEST(InMemoryRepositoryTest, SetStorage) {
  InMemoryRepository repository;
  EXPECT_OUTCOME_TRUE_1(repository.setStorage(
      ([](StorageConfig &cfg) { cfg.storage_paths.push_back({"test1"}); })));
  EXPECT_OUTCOME_TRUE(path, repository.path())
  EXPECT_OUTCOME_TRUE(config, repository.getStorage())
  std::vector<LocalPath> paths = {{path.string()}, {"test1"}};
  ASSERT_EQ(config.storage_paths, paths);
}
