/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/existing_selector.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "primitives/types.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/mocks/sector_storage/worker_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::sector_storage {
  using primitives::StoragePath;
  using primitives::WorkerResources;
  using stores::StorageInfo;
  using testing::_;

  class ExistingSelectorTest : public ::testing::Test {
   protected:
    void SetUp() override {
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;
      index_ = std::make_unique<stores::SectorIndexMock>();

      sector_ = SectorId{
          .miner = 42,
          .sector = 1,
      };

      file_type_ = SectorFileType::FTUnsealed;

      worker_ = std::make_shared<WorkerMock>();

      existing_selector_ = std::make_unique<ExistingSelector>(
          index_, sector_, file_type_, false);
    }

    std::shared_ptr<stores::SectorIndexMock> index_;
    SectorFileType file_type_;
    SectorId sector_;
    RegisteredSealProof seal_proof_type_;
    std::shared_ptr<WorkerMock> worker_;
    std::shared_ptr<ExistingSelector> existing_selector_;
  };

  /**
   * @given worker
   * @when try to check is worker can handle task, without supported task
   * @then getting false
   */
  TEST_F(ExistingSelectorTest, NotSupportedTask) {
    std::shared_ptr<WorkerHandle> worker_handle =
        std::make_shared<WorkerHandle>();

    worker_handle->worker = worker_;

    EXPECT_CALL(*worker_, getSupportedTask())
        .WillOnce(testing::Return(outcome::success(std::set<TaskType>())));

    EXPECT_OUTCOME_EQ(
        existing_selector_->is_satisfying(
            primitives::kTTAddPiece, seal_proof_type_, worker_handle),
        false);
  }

  /**
   * @given worker
   * @when try to check is worker can handle task, without have sector
   * @then getting false
   */
  TEST_F(ExistingSelectorTest, NotSector) {
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

    EXPECT_CALL(*index_, storageFindSector(sector_, file_type_, _))
        .WillOnce(
            testing::Return(outcome::success(std::vector({index_storage}))));

    EXPECT_OUTCOME_EQ(
        existing_selector_->is_satisfying(
            primitives::kTTAddPiece, seal_proof_type_, worker_handle),
        false);
  }

  /**
   * @given worker
   * @when try to check is worker can handle task
   * @then getting true
   */
  TEST_F(ExistingSelectorTest, WorkerSatisfy) {
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

    EXPECT_CALL(*index_, storageFindSector(sector_, file_type_, _))
        .WillOnce(
            testing::Return(outcome::success(std::vector({index_storage}))));

    EXPECT_OUTCOME_EQ(
        existing_selector_->is_satisfying(
            primitives::kTTAddPiece, seal_proof_type_, worker_handle),
        true);
  }

  /**
   * @given worker
   * @when try to check is worker can handle task
   * @then getting true
   * @note here selector can fetch
   */
  TEST_F(ExistingSelectorTest, WorkerSatisfyWithFetch) {
    auto existing_selector{
        std::make_unique<ExistingSelector>(index_, sector_, file_type_, true)};

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

    EXPECT_OUTCOME_TRUE(sector_size, getSectorSize(seal_proof_type_));

    EXPECT_CALL(*index_,
                storageFindSector(
                    sector_, file_type_, boost::make_optional(sector_size)))
        .WillOnce(
            testing::Return(outcome::success(std::vector({index_storage}))));

    EXPECT_OUTCOME_EQ(
        existing_selector->is_satisfying(
            primitives::kTTAddPiece, seal_proof_type_, worker_handle),
        true);
  }

  /**
   * @given 2 worker handle(best and some)
   * @when try to check is some better than best
   * @then getting false
   */
  TEST_F(ExistingSelectorTest, WorkersCompare) {
    std::shared_ptr<WorkerHandle> best_handle =
        std::make_shared<WorkerHandle>();

    best_handle->info.resources = WorkerResources{
        .physical_memory = 4096,
        .swap_memory = 0,
        .reserved_memory = 0,
        .cpus = 6,
        .gpus = {},
    };

    best_handle->active.memory_used_min = 10;

    std::shared_ptr<WorkerHandle> some_handle =
        std::make_shared<WorkerHandle>();

    some_handle->info.resources = WorkerResources{
        .physical_memory = 2048,
        .swap_memory = 0,
        .reserved_memory = 0,
        .cpus = 4,
        .gpus = {},
    };

    some_handle->active.memory_used_min = 5;
    EXPECT_OUTCOME_EQ(existing_selector_->is_preferred(
                          primitives::kTTAddPiece, some_handle, best_handle),
                      false);
  }
}  // namespace fc::sector_storage
