/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/task_selector.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/error_text.hpp"
#include "primitives/types.hpp"
#include "testutil/mocks/sector_storage/worker_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::sector_storage {
  using primitives::WorkerResources;
  using testing::_;

  class TaskSelectorTest : public ::testing::Test {
   protected:
    void SetUp() override {
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;

      worker_ = std::make_shared<WorkerMock>();

      task_selector_ = std::make_unique<TaskSelector>();
    }

    RegisteredSealProof seal_proof_type_;
    std::shared_ptr<WorkerMock> worker_;
    std::shared_ptr<TaskSelector> task_selector_;
  };

  /**
   * @given worker
   * @when try to check is worker can handle task, but getSupportedTask return
   * error
   * @then getting this error
   */
  TEST_F(TaskSelectorTest, isSatisfyingOutcomeError) {
    std::shared_ptr<WorkerHandle> worker_handle =
        std::make_shared<WorkerHandle>();

    worker_handle->worker = worker_;

    EXPECT_CALL(*worker_, getSupportedTask())
        .WillOnce(testing::Return(outcome::failure(ERROR_TEXT("ERROR"))));

    EXPECT_OUTCOME_FALSE_1(task_selector_->is_satisfying(
        primitives::kTTAddPiece, seal_proof_type_, worker_handle));
  }

  /**
   * @given worker
   * @when try to check is worker can handle task, without supported task
   * @then getting false
   */
  TEST_F(TaskSelectorTest, NotSupportedTask) {
    std::shared_ptr<WorkerHandle> worker_handle =
        std::make_shared<WorkerHandle>();

    worker_handle->worker = worker_;

    EXPECT_CALL(*worker_, getSupportedTask())
        .WillOnce(testing::Return(outcome::success(std::set<TaskType>())));

    EXPECT_OUTCOME_EQ(
        task_selector_->is_satisfying(
            primitives::kTTAddPiece, seal_proof_type_, worker_handle),
        false);
  }

  /**
   * @given worker
   * @when try to check is worker can handle task
   * @then getting true
   */
  TEST_F(TaskSelectorTest, WorkerSatisfy) {
    std::shared_ptr<WorkerHandle> worker_handle =
        std::make_shared<WorkerHandle>();

    worker_handle->worker = worker_;

    EXPECT_CALL(*worker_, getSupportedTask())
        .WillOnce(testing::Return(
            outcome::success(std::set<TaskType>({primitives::kTTAddPiece}))));

    EXPECT_OUTCOME_EQ(
        task_selector_->is_satisfying(
            primitives::kTTAddPiece, seal_proof_type_, worker_handle),
        true);
  }

  /**
   * @given 2 worker handle(best(1 task) and some(2 task))
   * @when try to check is some better than best
   * @then getting false
   */
  TEST_F(TaskSelectorTest, WorkersCompareTask) {
    std::shared_ptr<WorkerHandle> best_handle =
        std::make_shared<WorkerHandle>();

    EXPECT_CALL(*worker_, getSupportedTask())
        .WillOnce(testing::Return(
            outcome::success(std::set<TaskType>({primitives::kTTAddPiece}))));

    best_handle->worker = worker_;

    std::shared_ptr<WorkerHandle> some_handle =
        std::make_shared<WorkerHandle>();

    auto one_more_worker{std::make_shared<WorkerMock>()};

    EXPECT_CALL(*one_more_worker, getSupportedTask())
        .WillOnce(testing::Return(outcome::success(std::set<TaskType>(
            {primitives::kTTAddPiece, primitives::kTTUnseal}))));

    some_handle->worker = one_more_worker;

    EXPECT_OUTCOME_EQ(task_selector_->is_preferred(
                          primitives::kTTAddPiece, some_handle, best_handle),
                      false);
  }

  /**
   * @given 2 worker handle(not_best(1 task) and some(1 task))
   * @when try to check is some better than best
   * @then getting true
   */
  TEST_F(TaskSelectorTest, WorkersCompare) {
    std::shared_ptr<WorkerHandle> not_best_handle =
        std::make_shared<WorkerHandle>();

    not_best_handle->info.resources = WorkerResources{
        .physical_memory = 1024,
        .swap_memory = 0,
        .reserved_memory = 0,
        .cpus = 6,
        .gpus = {},
    };

    not_best_handle->active.setMemoryUsedMin(2048);

    EXPECT_CALL(*worker_, getSupportedTask())
        .WillOnce(testing::Return(
            outcome::success(std::set<TaskType>({primitives::kTTAddPiece}))));

    not_best_handle->worker = worker_;

    std::shared_ptr<WorkerHandle> some_handle =
        std::make_shared<WorkerHandle>();

    auto one_more_worker{std::make_shared<WorkerMock>()};

    EXPECT_CALL(*one_more_worker, getSupportedTask())
        .WillOnce(testing::Return(
            outcome::success(std::set<TaskType>({primitives::kTTAddPiece}))));

    some_handle->worker = one_more_worker;

    some_handle->info.resources = WorkerResources{
        .physical_memory = 1024,
        .swap_memory = 0,
        .reserved_memory = 0,
        .cpus = 6,
        .gpus = {},
    };

    some_handle->active.setMemoryUsedMin(1024);

    EXPECT_OUTCOME_EQ(
        task_selector_->is_preferred(
            primitives::kTTAddPiece, some_handle, not_best_handle),
        true);
  }
}  // namespace fc::sector_storage
