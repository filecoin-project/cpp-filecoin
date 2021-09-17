/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/allocate_selector.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "primitives/types.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/mocks/sector_storage/worker_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::sector_storage {
  using primitives::StoragePath;
  using primitives::SectorSize;
  using primitives::WorkerResources;
  using stores::StorageInfo;
  using testing::_;

  class AllocateSelectorTest : public ::testing::Test {
   protected:
    void SetUp() override {
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;
      index_ = std::make_unique<stores::SectorIndexMock>();

      EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));

      sector_size_ = size;

      file_type_ = SectorFileType::FTUnsealed;

      path_type_ = PathType::kStorage;

      worker_ = std::make_shared<WorkerMock>();

      allocate_selector_ =
          std::make_unique<AllocateSelector>(index_, file_type_, path_type_);
    }


    std::shared_ptr<stores::SectorIndexMock> index_;
    SectorFileType file_type_;
    PathType path_type_;
    RegisteredSealProof seal_proof_type_;
    SectorSize sector_size_;
    std::shared_ptr<WorkerMock> worker_;
    std::shared_ptr<AllocateSelector> allocate_selector_;
  };

  /**
   * @given worker
   * @when try to check is worker can handle task, without supported task
   * @then getting false
   */
  TEST_F(AllocateSelectorTest, NotSupportedTask) {
    std::shared_ptr<WorkerHandle> worker_handle =
        std::make_shared<WorkerHandle>();

    worker_handle->worker = worker_;

    EXPECT_CALL(*worker_, getSupportedTask())
        .WillOnce(testing::Return(outcome::success(std::set<TaskType>())));

    EXPECT_OUTCOME_EQ(
        allocate_selector_->is_satisfying(
            primitives::kTTAddPiece, seal_proof_type_, worker_handle),
        false);
  }

  /**
   * @given worker
   * @when try to check is worker can handle task, without have sector
   * @then getting false
   */
  TEST_F(AllocateSelectorTest, NotSector) {
    std::shared_ptr<WorkerHandle> worker_handle =
        std::make_shared<WorkerHandle>();

    worker_handle->worker = worker_;

    EXPECT_CALL(*worker_, getSupportedTask())
        .WillOnce(testing::Return(
            outcome::success(std::set<TaskType>({primitives::kTTAddPiece}))));

    StoragePath worker_storage{
        .id = "worker storage id",
    };

    EXPECT_CALL(*worker_, getAccessiblePaths())
        .WillOnce(
            testing::Return(outcome::success(std::vector({worker_storage}))));

    StorageInfo index_storage{
        .id = "index storage id",
    };

    EXPECT_CALL(*index_, storageBestAlloc(file_type_, sector_size_, false))
        .WillOnce(
            testing::Return(outcome::success(std::vector({index_storage}))));

    EXPECT_OUTCOME_EQ(
        allocate_selector_->is_satisfying(
            primitives::kTTAddPiece, seal_proof_type_, worker_handle),
        false);
  }

  /**
   * @given worker
   * @when try to check is worker can handle task
   * @then getting true
   */
  TEST_F(AllocateSelectorTest, WorkerSatisfy) {
    std::shared_ptr<WorkerHandle> worker_handle =
        std::make_shared<WorkerHandle>();

    worker_handle->worker = worker_;

    EXPECT_CALL(*worker_, getSupportedTask())
        .WillOnce(testing::Return(
            outcome::success(std::set<TaskType>({primitives::kTTAddPiece}))));

    primitives::StorageID id = "storage id";

    StoragePath worker_storage{
        .id = id,
    };

    EXPECT_CALL(*worker_, getAccessiblePaths())
        .WillOnce(
            testing::Return(outcome::success(std::vector({worker_storage}))));

    StorageInfo index_storage{
        .id = id,
    };

    EXPECT_CALL(*index_, storageBestAlloc(file_type_, sector_size_, false))
        .WillOnce(
            testing::Return(outcome::success(std::vector({index_storage}))));

    EXPECT_OUTCOME_EQ(
        allocate_selector_->is_satisfying(
            primitives::kTTAddPiece, seal_proof_type_, worker_handle),
        true);
  }

  /**
   * @given 2 worker handle(best and some)
   * @when try to check is some better than best
   * @then getting false
   */
  TEST_F(AllocateSelectorTest, WorkersCompare) {
    std::shared_ptr<WorkerHandle> best_handle =
        std::make_shared<WorkerHandle>();

    best_handle->info.resources = WorkerResources{
        .physical_memory = 4096,
        .swap_memory = 0,
        .reserved_memory = 0,
        .cpus = 6,
        .gpus = {},
    };

    best_handle->active.setMemoryUsedMin(10);

    std::shared_ptr<WorkerHandle> some_handle =
        std::make_shared<WorkerHandle>();

    some_handle->info.resources = WorkerResources{
        .physical_memory = 2048,
        .swap_memory = 0,
        .reserved_memory = 0,
        .cpus = 4,
        .gpus = {},
    };

    some_handle->active.setMemoryUsedMin(5);
    EXPECT_OUTCOME_EQ(allocate_selector_->is_preferred(
                          primitives::kTTAddPiece, some_handle, best_handle),
                      false);
  }

}  // namespace fc::sector_storage
