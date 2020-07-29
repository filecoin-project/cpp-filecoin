/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/scheduler_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include "testutil/mocks/sector_storage/selector_mock.hpp"
#include "testutil/outcome.hpp"

using fc::primitives::WorkerInfo;
using fc::primitives::WorkerResources;
using fc::sector_storage::Scheduler;
using fc::sector_storage::SchedulerImpl;
using fc::sector_storage::SelectorMock;
using fc::sector_storage::Worker;
using fc::sector_storage::WorkerAction;
using fc::sector_storage::WorkerHandle;
using ::testing::_;

MATCHER_P(workerNameMatcher, worker_name, "compare workers name") {
  return (arg->info.hostname == worker_name);
}

class SchedulerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    seal_proof_type_ = RegisteredProof::StackedDRG2KiBSeal;

    scheduler_ = std::make_unique<SchedulerImpl>(seal_proof_type_);

    std::unique_ptr<WorkerHandle> worker = std::make_unique<WorkerHandle>();

    worker_name_ = "worker";

    worker->info = WorkerInfo{
        .hostname = worker_name_,
        .resources = WorkerResources{.physical_memory = uint64_t(1) << 20,
                                     .swap_memory = 0,
                                     .reserved_memory = 0,
                                     .cpus = 0,
                                     .gpus = {}}};
    scheduler_->newWorker(std::move(worker));

    selector_ = std::make_shared<SelectorMock>();
  }

  std::string worker_name_;

  RegisteredProof seal_proof_type_;
  std::shared_ptr<SelectorMock> selector_;
  std::unique_ptr<Scheduler> scheduler_;
};

/**
 * @given Task data
 * @when when try to schedule it
 * @then work is done
 */
TEST_F(SchedulerTest, ScheuleTask) {
  uint64_t counter = 0;

  WorkerAction prepare = [&](const std::shared_ptr<Worker> &worker) {
    EXPECT_EQ(counter++, 0);
    return fc::outcome::success();
  };
  WorkerAction work = [&](const std::shared_ptr<Worker> &worker) {
    EXPECT_EQ(counter++, 1);
    return fc::outcome::success();
  };

  SectorId sector{
      .miner = 42,
      .sector = 1,
  };

  auto task = fc::primitives::kTTFinalize;
  EXPECT_CALL(
      *selector_,
      is_satisfying(task, seal_proof_type_, workerNameMatcher(worker_name_)))
      .WillOnce(testing::Return(fc::outcome::success(true)));

  EXPECT_OUTCOME_TRUE_1(
      scheduler_->schedule(sector, task, selector_, prepare, work))

  EXPECT_EQ(counter, 2);
}

/**
 * @given 2 tasks data
 * @when when try to schedule first, it will be in request queue
 * the second will processed
 * @then worker gets first from request queue and process it
 */
TEST_F(SchedulerTest, RequestQueue) {
  std::unique_ptr<WorkerHandle> worker1 = std::make_unique<WorkerHandle>();

  std::string worker1_name_ = "everything";

  worker1->info =
      WorkerInfo{.hostname = worker1_name_, .resources = WorkerResources{}};

  EXPECT_CALL(*selector_, is_satisfying(_, _, workerNameMatcher("everything")))
      .WillRepeatedly(testing::Return(fc::outcome::success(true)));

  EXPECT_CALL(
      *selector_,
      is_preferred(
          _, workerNameMatcher(worker_name_), workerNameMatcher(worker1_name_)))
      .WillRepeatedly(testing::Return(fc::outcome::success(true)));

  EXPECT_CALL(*selector_,
              is_preferred(_,
                           workerNameMatcher(worker1_name_),
                           workerNameMatcher(worker_name_)))
      .WillRepeatedly(
          testing::Return(fc::outcome::success(false)));  // Just reverse

  scheduler_->newWorker(std::move(worker1));

  uint64_t counter = 0;

  WorkerAction prepare1 = [&](const std::shared_ptr<Worker> &worker) {
    EXPECT_EQ(counter++, 2);
    return fc::outcome::success();
  };
  WorkerAction work1 = [&](const std::shared_ptr<Worker> &worker) {
    EXPECT_EQ(counter++, 3);
    return fc::outcome::success();
  };

  WorkerAction prepare2 = [&](const std::shared_ptr<Worker> &worker) {
    EXPECT_EQ(counter++, 0);
    return fc::outcome::success();
  };
  WorkerAction work2 = [&](const std::shared_ptr<Worker> &worker) {
    EXPECT_EQ(counter++, 1);
    return fc::outcome::success();
  };

  SectorId sector{
      .miner = 42,
      .sector = 1,
  };
  auto task1 = fc::primitives::kTTReadUnsealed;
  EXPECT_CALL(
      *selector_,
      is_satisfying(task1, seal_proof_type_, workerNameMatcher(worker_name_)))
      .WillOnce(testing::Return(fc::outcome::success(false)))
      .WillOnce(testing::Return(fc::outcome::success(true)))
      .WillOnce(testing::Return(fc::outcome::success(true)));
  bool thread_error;
  std::thread t([&]() {
    auto maybe_err =
        scheduler_->schedule(sector, task1, selector_, prepare1, work1);

    thread_error = maybe_err.has_error();
  });

  auto task2 = fc::primitives::kTTFinalize;
  EXPECT_CALL(
      *selector_,
      is_satisfying(task2, seal_proof_type_, workerNameMatcher(worker_name_)))
      .WillOnce(testing::Return(fc::outcome::success(true)));

  EXPECT_OUTCOME_TRUE_1(
      scheduler_->schedule(sector, task2, selector_, prepare2, work2))

  t.join();
  ASSERT_FALSE(thread_error);
  EXPECT_EQ(counter, 4);
}
