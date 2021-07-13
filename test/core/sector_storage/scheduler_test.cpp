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

namespace fc::sector_storage {
  using primitives::WorkerInfo;
  using primitives::WorkerResources;
  using ::testing::_;

  MATCHER_P(workerNameMatcher, worker_name, "compare workers name") {
    return (arg->info.hostname == worker_name);
  }

  class SchedulerTest : public ::testing::Test {
   protected:
    void SetUp() override {
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;

      io_ = std::make_shared<boost::asio::io_context>();

      scheduler_ = std::make_unique<SchedulerImpl>(io_);

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

    std::shared_ptr<boost::asio::io_context> io_;
    RegisteredSealProof seal_proof_type_;
    std::shared_ptr<SelectorMock> selector_;
    std::unique_ptr<Scheduler> scheduler_;
  };

  /**
   * @given Task data
   * @when when try to schedule it
   * @then work is done
   * @note Disabled due to the fact that there are not always threads in the
   * thread pool
   */
  TEST_F(SchedulerTest, ScheuleTask) {
    uint64_t counter = 0;

    SectorId sector_id{
        .miner = 42,
        .sector = 1,
    };
    SectorRef sector{
        .id = sector_id,
        .proof_type = seal_proof_type_,
    };

    CallId call_id{.sector = sector_id, .id = "someUUID"};
    WorkerAction prepare = [&](const std::shared_ptr<Worker> &worker) {
      EXPECT_EQ(counter++, 0);
      return outcome::success(call_id);
    };

    CallId call_id2{.sector = sector_id, .id = "someUUID2"};
    WorkerAction work = [&](const std::shared_ptr<Worker> &worker) {
      EXPECT_EQ(counter++, 1);
      return outcome::success(call_id2);
    };
    ReturnCb cb = [&](const outcome::result<CallResult> &) {
      EXPECT_EQ(counter++, 2);
    };

    auto task = primitives::kTTFinalize;
    EXPECT_CALL(
        *selector_,
        is_satisfying(task, seal_proof_type_, workerNameMatcher(worker_name_)))
        .WillOnce(testing::Return(outcome::success(true)));

    EXPECT_OUTCOME_TRUE_1(
        scheduler_->schedule(sector, task, selector_, prepare, work, cb));

    io_->run_one();
    io_->reset();

    EXPECT_OUTCOME_TRUE_1(scheduler_->returnFetch(call_id, boost::none));
    EXPECT_OUTCOME_TRUE_1(
        scheduler_->returnFinalizeSector(call_id2, boost::none));

    io_->run_one();
    io_->run_one();

    EXPECT_EQ(counter, 3);
  }

  /**
   * @given 2 tasks data
   * @when when try to schedule first, it will be in request queue
   * the second will processed
   * @then worker gets first from request queue and process it
   * @note Disabled due to the fact that there are not always threads in the
   * thread pool
   */
  TEST_F(SchedulerTest, RequestQueue) {
    std::unique_ptr<WorkerHandle> worker1 = std::make_unique<WorkerHandle>();

    std::string worker1_name_ = "everything";

    worker1->info =
        WorkerInfo{.hostname = worker1_name_, .resources = WorkerResources{}};

    EXPECT_CALL(*selector_,
                is_satisfying(_, _, workerNameMatcher("everything")))
        .WillRepeatedly(testing::Return(outcome::success(true)));

    EXPECT_CALL(*selector_,
                is_preferred(_,
                             workerNameMatcher(worker_name_),
                             workerNameMatcher(worker1_name_)))
        .WillRepeatedly(testing::Return(outcome::success(true)));

    EXPECT_CALL(*selector_,
                is_preferred(_,
                             workerNameMatcher(worker1_name_),
                             workerNameMatcher(worker_name_)))
        .WillRepeatedly(
            testing::Return(outcome::success(false)));  // Just reverse

    scheduler_->newWorker(std::move(worker1));

    uint64_t counter = 0;

    SectorId sector_id{
        .miner = 42,
        .sector = 1,
    };
    SectorRef sector{
        .id = sector_id,
        .proof_type = seal_proof_type_,
    };

    CallId call_id{.sector = sector_id, .id = "UUID1"};
    CallId call_id2{.sector = sector_id, .id = "UUID2"};
    WorkerAction prepare1 = [&](const std::shared_ptr<Worker> &worker) {
      EXPECT_EQ(counter++, 3);
      return outcome::success(call_id);
    };
    WorkerAction work1 = [&](const std::shared_ptr<Worker> &worker) {
      EXPECT_EQ(counter++, 4);
      return outcome::success(call_id2);
    };
    ReturnCb cb1 = [&](const outcome::result<CallResult> &) {
      EXPECT_EQ(counter++, 5);
    };

    CallId call_id3{.sector = sector_id, .id = "UUID3"};
    CallId call_id4{.sector = sector_id, .id = "UUID4"};
    WorkerAction prepare2 = [&](const std::shared_ptr<Worker> &worker) {
      EXPECT_EQ(counter++, 0);
      return outcome::success(call_id3);
    };
    WorkerAction work2 = [&](const std::shared_ptr<Worker> &worker) {
      EXPECT_EQ(counter++, 1);
      return outcome::success(call_id4);
    };
    ReturnCb cb2 = [&](const outcome::result<CallResult> &) {
      EXPECT_EQ(counter++, 2);
    };

    auto task1 = primitives::kTTReadUnsealed;
    EXPECT_CALL(
        *selector_,
        is_satisfying(task1, seal_proof_type_, workerNameMatcher(worker_name_)))
        .WillOnce(testing::Return(outcome::success(false)))
        .WillOnce(testing::Return(outcome::success(true)))
        .WillOnce(testing::Return(outcome::success(true)));

    EXPECT_OUTCOME_TRUE_1(
        scheduler_->schedule(sector, task1, selector_, prepare1, work1, cb1));

    auto task2 = primitives::kTTFinalize;
    EXPECT_CALL(
        *selector_,
        is_satisfying(task2, seal_proof_type_, workerNameMatcher(worker_name_)))
        .WillOnce(testing::Return(outcome::success(true)));

    EXPECT_OUTCOME_TRUE_1(
        scheduler_->schedule(sector, task2, selector_, prepare2, work2, cb2));

    io_->run_one();
    EXPECT_OUTCOME_TRUE_1(
        scheduler_->returnFinalizeSector(call_id3, boost::none));
    io_->reset();
    io_->run_one();
    EXPECT_OUTCOME_TRUE_1(
        scheduler_->returnFinalizeSector(call_id4, boost::none));
    io_->reset();
    io_->run_one();
    EXPECT_OUTCOME_TRUE_1(
        scheduler_->returnFinalizeSector(call_id, boost::none));
    io_->run_one();
    EXPECT_OUTCOME_TRUE_1(
        scheduler_->returnFinalizeSector(call_id2, boost::none));
    io_->run_one();
    io_->run_one();
    EXPECT_EQ(counter, 6);
  }
}  // namespace fc::sector_storage
