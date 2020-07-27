/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/manager_impl.hpp"

#include "testutil/mocks/sector_storage/scheduler_mock.hpp"
#include "testutil/mocks/sector_storage/stores/local_storage_mock.hpp"
#include "testutil/mocks/sector_storage/stores/local_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/remote_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/outcome.hpp"

using fc::primitives::StoragePath;
using fc::sector_storage::Manager;
using fc::sector_storage::ManagerImpl;
using fc::sector_storage::SchedulerMock;
using fc::sector_storage::SealerConfig;
using fc::sector_storage::stores::LocalStorageMock;
using fc::sector_storage::stores::LocalStoreMock;
using fc::sector_storage::stores::RemoteStoreMock;
using fc::sector_storage::stores::SectorIndexMock;
using ::testing::_;

class ManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    {
      home_dir_ = "/home";
      char *home = std::getenv("HOME");
      if (home) {
        old_home_dir_ = home;
      } else {
        old_home_dir_ = boost::none;
      }
      setenv("HOME", home_dir_.c_str(), 1);
    }

    seal_proof_type_ = RegisteredProof::StackedDRG2KiBSeal;

    sector_index_ = std::make_shared<SectorIndexMock>();

    local_storage_ = std::make_shared<LocalStorageMock>();

    local_store_ = std::make_shared<LocalStoreMock>();

    EXPECT_CALL(*local_store_, getSectorIndex())
        .WillRepeatedly(::testing::Return(sector_index_));

    EXPECT_CALL(*local_store_, getLocalStorage())
        .WillRepeatedly(::testing::Return(local_storage_));

    remote_store_ = std::make_shared<RemoteStoreMock>();

    EXPECT_CALL(*remote_store_, getLocalStore())
        .WillRepeatedly(::testing::Return(local_store_));

    EXPECT_CALL(*remote_store_, getSectorIndex())
        .WillRepeatedly(::testing::Return(sector_index_));

    scheduler_ = std::make_shared<SchedulerMock>();

    EXPECT_CALL(*scheduler_, getSealProofType())
        .WillRepeatedly(::testing::Return(seal_proof_type_));

    EXPECT_CALL(*scheduler_, doNewWorker(_))
        .WillRepeatedly(::testing::Return());

    auto maybe_manager = ManagerImpl::newManager(remote_store_,
                                                 scheduler_,
                                                 SealerConfig{

                                                     .allow_precommit_1 = true,
                                                     .allow_precommit_2 = true,
                                                     .allow_commit = true,
                                                     .allow_unseal = true,
                                                 });
    EXPECT_FALSE(maybe_manager.has_error())
        << "Manager init is failed: " << maybe_manager.error().message();
    manager_ = std::move(maybe_manager.value());
  }

  void TearDown() override {
    if (old_home_dir_) {
      setenv("HOME", (*old_home_dir_).c_str(), 1);
    } else {
      unsetenv("HOME");
    }
  }

 protected:
  RegisteredProof seal_proof_type_;

  std::shared_ptr<SectorIndexMock> sector_index_;
  std::shared_ptr<LocalStorageMock> local_storage_;
  std::shared_ptr<LocalStoreMock> local_store_;
  std::shared_ptr<RemoteStoreMock> remote_store_;
  std::shared_ptr<SchedulerMock> scheduler_;
  std::unique_ptr<Manager> manager_;

  std::string home_dir_;

 private:
  boost::optional<std::string> old_home_dir_;
};

/**
 * @given absolute path
 * @when when try to add new local storage
 * @then opened this path
 */
TEST_F(ManagerTest, addLocalStorageWithoutExpand) {
  std::string path = "/some/path/here";

  EXPECT_CALL(*local_store_, openPath(path))
      .WillOnce(::testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_TRUE_1(manager_->addLocalStorage(path));
}

/**
 * @given path from home
 * @when when try to add new local storage
 * @then opened absolute path
 */
TEST_F(ManagerTest, addLocalStorageWithExpand) {
  std::string path = "~/some/path/here";
  std::string result = "/home/some/path/here";

  EXPECT_CALL(*local_store_, openPath(result))
      .WillOnce(::testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_TRUE_1(manager_->addLocalStorage(path));
}

/**
 * @given manager
 * @when when try to getLocalStorages
 * @then gets maps with id and paths
 */
TEST_F(ManagerTest, getLocalStorages) {
  std::vector<StoragePath> paths = {};
  std::unordered_map<StorageID, std::string> result = {};

  for (int i = 0; i < 5; i++) {
    std::string id = "id_" + std::to_string(i);
    std::string path = "/some/path/" + std::to_string(i);
    result[id] = path;
    paths.push_back(StoragePath{
        .id = id,
        .weight = 0,
        .local_path = path,
        .can_seal = false,
        .can_store = false,
    });
  }

  EXPECT_CALL(*local_store_, getAccessiblePaths())
      .WillOnce(::testing::Return(fc::outcome::success(paths)));

  EXPECT_OUTCOME_TRUE(storages, manager_->getLocalStorages());
  EXPECT_THAT(storages, ::testing::UnorderedElementsAreArray(result));
}
