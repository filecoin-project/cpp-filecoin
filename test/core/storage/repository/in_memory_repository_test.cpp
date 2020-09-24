/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/impl/in_memory_repository.hpp"

#include "storage/repository/repository_error.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::storage::repository::InMemoryRepository;
using fc::sector_storage::stores::LocalPath;
using fc::sector_storage::stores::StorageConfig;


class InMemoryRepositoryTest : public test::BaseFS_Test {
 public:
  InMemoryRepositoryTest()
      : test::BaseFS_Test("in_memory_repository_test") {}

  /**
   * Create a test repository with an empty file.
   */
  void SetUp() override {
    BaseFS_Test::SetUp();
  }
};

TEST_F(InMemoryRepositoryTest, getConfigTest) {
  InMemoryRepository repository;
  auto config_path = fs::canonical(createFile("storage.json")).string();
  std::ofstream config_file(config_path);
  config_file << "{\n"
                 "  \"StoragePaths\": [\n"
                 "    {\n"
                 "      \"Path\": \"preseal1\"\n"
                 "    },\n"
                 "    {\n"
                 "      \"Path\": \"miner1\"\n"
                 "    }\n"
                 "  ]\n"
                 "}";
  config_file.close();
  EXPECT_OUTCOME_TRUE(config, repository.getStorage());
  EXPECT_OUTCOME_TRUE(path, repository.path())
  std::vector<LocalPath> paths = {{path.string()}};
  ASSERT_EQ(config.storage_paths, paths);
}

TEST_F(InMemoryRepositoryTest, setConfigTest) {
  InMemoryRepository repository;
  auto config_path = fs::canonical(createFile("storage.json")).string();
  std::ofstream config_file(config_path);
  config_file << "{\n"
                 "  \"StoragePaths\": [\n"
                 "    {\n"
                 "      \"Path\": \"preseal1\"\n"
                 "    },\n"
                 "    {\n"
                 "      \"Path\": \"miner1\"\n"
                 "    }\n"
                 "  ]\n"
                 "}";
  config_file.close();
  EXPECT_OUTCOME_TRUE_1(repository.setStorage(([](StorageConfig& cfg) {
                          cfg.storage_paths.push_back({"test1"});
                        })));
  EXPECT_OUTCOME_TRUE(path, repository.path())
  EXPECT_OUTCOME_TRUE(config, repository.getStorage())
  std::vector<LocalPath> paths = {{path.string()}, {"test1"}};
  ASSERT_EQ(config.storage_paths, paths);
}