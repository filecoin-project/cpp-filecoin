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
#include "testutil/storage/base_fs_test.hpp"

using fc::sector_storage::Manager;
using fc::sector_storage::ManagerImpl;
using fc::sector_storage::SchedulerMock;
using fc::sector_storage::SealerConfig;
using fc::sector_storage::stores::LocalStorageMock;
using fc::sector_storage::stores::LocalStoreMock;
using fc::sector_storage::stores::RemoteStoreMock;
using fc::sector_storage::stores::SectorIndexMock;
using ::testing::_;

class ManagerTest : public test::BaseFS_Test {
 public:
  ManagerTest() : test::BaseFS_Test("fc_manager_test") {
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

 protected:
  RegisteredProof seal_proof_type_;

  std::shared_ptr<SectorIndexMock> sector_index_;
  std::shared_ptr<LocalStorageMock> local_storage_;
  std::shared_ptr<LocalStoreMock> local_store_;
  std::shared_ptr<RemoteStoreMock> remote_store_;
  std::shared_ptr<SchedulerMock> scheduler_;
  std::unique_ptr<Manager> manager_;
};
