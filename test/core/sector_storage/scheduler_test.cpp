/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/new_scheduler_impl.hpp"
#include "sector_storage/impl/scheduler_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/optional/optional_io.hpp>
#include <thread>
#include <utility>

#include "sector_storage/scheduler_utils.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/mocks/sector_storage/selector_mock.hpp"
#include "testutil/mocks/sector_storage/worker_estimator_mock.hpp"
#include "testutil/mocks/sector_storage/worker_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::sector_storage {
  using primitives::WorkerInfo;
  using primitives::WorkerResources;
  using storage::InMemoryStorage;
  using ::testing::_;

  MATCHER_P(workerNameMatcher, worker_name, "compare workers name") {
    return (arg->info.hostname == worker_name);
  }

  class SchedulerTest : public ::testing::Test {
   protected:
    void SetUp() override {
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;

      io_ = std::make_shared<boost::asio::io_context>();

      kv_ = std::make_shared<InMemoryStorage>();

      auto sector = SectorId{
          .miner = 42,
          .sector = 1,
      };
      WorkState ws;
      EXPECT_OUTCOME_TRUE(
          wid1, getWorkId(primitives::kTTPreCommit1, std::make_tuple(sector)));
      ws.id = wid1;
      ws.status = WorkStatus::kStart;
      EXPECT_OUTCOME_TRUE(raw1, codec::cbor::encode(ws));
      EXPECT_OUTCOME_TRUE_1(
          kv_->put(static_cast<Bytes>(wid1), std::move(raw1)));
      states_.push_back(ws);
      sector.sector++;
      EXPECT_OUTCOME_TRUE(
          wid2, getWorkId(primitives::kTTPreCommit1, std::make_tuple(sector)));
      CallId callid{.sector = sector, .id = "some"};
      ws.id = wid2;
      ws.status = WorkStatus::kInProgress;
      ws.call_id = callid;
      EXPECT_OUTCOME_TRUE(raw2, codec::cbor::encode(ws));
      EXPECT_OUTCOME_TRUE_1(
          kv_->put(static_cast<Bytes>(wid2), std::move(raw2)));
      states_.push_back(ws);
      sector.sector++;
      EXPECT_OUTCOME_TRUE(
          wid3, getWorkId(primitives::kTTPreCommit1, std::make_tuple(sector)));
      ws.id = wid3;
      ws.status = WorkStatus::kStart;
      ws.call_id = CallId();
      EXPECT_OUTCOME_TRUE(raw3, codec::cbor::encode(ws));
      EXPECT_OUTCOME_TRUE_1(
          kv_->put(static_cast<Bytes>(wid3), std::move(raw3)));
      states_.push_back(ws);

      EXPECT_OUTCOME_TRUE(scheduler, SchedulerImpl::newScheduler(io_, kv_));

      scheduler_ = scheduler;

      std::unique_ptr<WorkerHandle> worker = std::make_unique<WorkerHandle>();
      mock_worker_ = std::make_unique<WorkerMock>();

      worker_name_ = "worker";
      worker->worker = mock_worker_;
      worker->info =
          WorkerInfo{.hostname = worker_name_,
                     .resources = WorkerResources{.physical_memory = 1ul << 20,
                                                  .swap_memory = 0,
                                                  .reserved_memory = 0,
                                                  .cpus = 0,
                                                  .gpus = {}}};
      scheduler_->newWorker(std::move(worker));

      selector_ = std::make_shared<SelectorMock>();
    }

    std::string worker_name_;
    std::vector<WorkState> states_;
    std::shared_ptr<WorkerMock> mock_worker_;
    std::shared_ptr<boost::asio::io_context> io_;
    RegisteredSealProof seal_proof_type_;
    std::shared_ptr<InMemoryStorage> kv_;
    std::shared_ptr<SelectorMock> selector_;
    std::shared_ptr<Scheduler> scheduler_;
  };

  /**
   * @given Task data
   * @when when try to schedule it
   * @then work is done
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

    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(sector,
                                               task,
                                               selector_,
                                               prepare,
                                               work,
                                               cb,
                                               kDefaultTaskPriority,
                                               boost::none));

    io_->run_one();
    io_->reset();

    EXPECT_OUTCOME_TRUE_1(scheduler_->returnResult(call_id, {}));
    EXPECT_OUTCOME_TRUE_1(scheduler_->returnResult(call_id2, {}));

    io_->run_one();
    io_->run_one();

    EXPECT_EQ(counter, 3);
  }

  /**
   * @given data in storage
   * @when when try to recover and get result
   * @then immediately return result
   */
  TEST_F(SchedulerTest, ResultAfterRestart) {
    CallId call_id{};
    WorkId work_id{};
    for (auto &ws : states_) {
      if (ws.status == WorkStatus::kInProgress) {
        work_id = ws.id;
        call_id = ws.call_id;
        ASSERT_TRUE(kv_->contains(static_cast<Bytes>(ws.id)));
      } else {
        ASSERT_FALSE(kv_->contains(static_cast<Bytes>(ws.id)));
      }
    }

    EXPECT_OUTCOME_TRUE_1(scheduler_->returnResult(call_id, {}));

    io_->run_one();

    bool is_called = false;
    ReturnCb cb = [&](const outcome::result<CallResult> &) {
      is_called = true;
    };

    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(SectorRef{},
                                               primitives::kTTPreCommit1,
                                               selector_,
                                               WorkerAction(),
                                               WorkerAction(),
                                               cb,
                                               kDefaultTaskPriority,
                                               work_id));

    ASSERT_TRUE(is_called);
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
        .WillOnce(testing::Return(outcome::success(true)));

    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(sector,
                                               task1,
                                               selector_,
                                               prepare1,
                                               work1,
                                               cb1,
                                               kDefaultTaskPriority,
                                               boost::none));

    auto task2 = primitives::kTTFinalize;
    EXPECT_CALL(
        *selector_,
        is_satisfying(task2, seal_proof_type_, workerNameMatcher(worker_name_)))
        .WillOnce(testing::Return(outcome::success(true)));

    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(sector,
                                               task2,
                                               selector_,
                                               prepare2,
                                               work2,
                                               cb2,
                                               kDefaultTaskPriority,
                                               boost::none));

    io_->run_one();
    EXPECT_OUTCOME_TRUE_1(scheduler_->returnResult(call_id3, {}));
    io_->reset();
    io_->run_one();
    EXPECT_OUTCOME_TRUE_1(scheduler_->returnResult(call_id4, {}));
    io_->reset();
    io_->run_one();
    EXPECT_OUTCOME_TRUE_1(scheduler_->returnResult(call_id, {}));
    io_->run_one();
    EXPECT_OUTCOME_TRUE_1(scheduler_->returnResult(call_id2, {}));
    io_->run_one();
    io_->run_one();
    EXPECT_EQ(counter, 6);
  }

  /**
   * @given sealing task data, work id
   * @when when try to schedule twice same task
   * @then first works, second just changes cb
   */
  TEST_F(SchedulerTest, ScheuleDuplicateTask) {
    auto task = primitives::kTTPreCommit1;
    SectorId sector_id{
        .miner = 42,
        .sector = 1,
    };
    SectorRef sector{
        .id = sector_id,
        .proof_type = seal_proof_type_,
    };
    EXPECT_OUTCOME_TRUE(work_id, getWorkId(task, std::make_tuple(sector)));

    CallId call_id{.sector = sector_id, .id = "someUUID"};
    WorkerAction work = [&](const std::shared_ptr<Worker> &worker) {
      return outcome::success(call_id);
    };

    bool is_first_called = false;
    ReturnCb cb = [&](const outcome::result<CallResult> &) {
      is_first_called = true;
    };

    EXPECT_CALL(*mock_worker_, isLocalWorker())
        .WillOnce(testing::Return(false));

    EXPECT_CALL(
        *selector_,
        is_satisfying(task, seal_proof_type_, workerNameMatcher(worker_name_)))
        .WillOnce(testing::Return(outcome::success(true)));

    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(sector,
                                               task,
                                               selector_,
                                               WorkerAction(),
                                               work,
                                               cb,
                                               kDefaultTaskPriority,
                                               work_id));

    io_->run_one();

    bool is_second_called = false;
    ReturnCb new_cb = [&](const outcome::result<CallResult> &) {
      is_second_called = true;
    };

    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(sector,
                                               task,
                                               selector_,
                                               WorkerAction(),
                                               work,
                                               new_cb,
                                               kDefaultTaskPriority,
                                               work_id));

    EXPECT_OUTCOME_TRUE_1(scheduler_->returnResult(call_id, {}));
    io_->reset();
    io_->run_one();

    ASSERT_FALSE(is_first_called);
    ASSERT_TRUE(is_second_called);
  }

  /**
   * @given 2 Task data
   * @when when try to schedule them together
   * @then they do not block each other
   */
  TEST_F(SchedulerTest, Scheule2Task) {
    SectorId sector_id1{
        .miner = 42,
        .sector = 1,
    };
    SectorRef sector1{
        .id = sector_id1,
        .proof_type = seal_proof_type_,
    };

    auto task = primitives::kTTFinalize;
    EXPECT_CALL(
        *selector_,
        is_satisfying(task, seal_proof_type_, workerNameMatcher(worker_name_)))
        .WillOnce(testing::Return(outcome::success(true)))
        .WillOnce(testing::Return(outcome::success(true)));

    SectorId sector_id2{
        .miner = 42,
        .sector = 2,
    };
    SectorRef sector2{
        .id = sector_id2,
        .proof_type = seal_proof_type_,
    };

    std::promise<void> work2_done;

    CallId call_id1{.sector = sector_id1, .id = "UUID1"};
    WorkerAction work1 = [&](const std::shared_ptr<Worker> &worker) {
      work2_done.get_future().get();
      return outcome::success(call_id1);
    };
    bool cb1_call = false;
    ReturnCb cb1 = [&](const outcome::result<CallResult> &res) {
      cb1_call = not res.has_error();
    };

    CallId call_id2{.sector = sector_id2, .id = "UUID2"};
    WorkerAction work2 = [&](const std::shared_ptr<Worker> &worker) {
      work2_done.set_value();
      return outcome::success(call_id2);
    };

    bool cb2_call = false;
    ReturnCb cb2 = [&](const outcome::result<CallResult> &res) {
      cb2_call = not res.has_error();
    };

    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(sector1,
                                               task,
                                               selector_,
                                               WorkerAction(),
                                               work1,
                                               cb1,
                                               kDefaultTaskPriority,
                                               boost::none));
    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(sector2,
                                               task,
                                               selector_,
                                               WorkerAction(),
                                               work2,
                                               cb2,
                                               kDefaultTaskPriority,
                                               boost::none));

    std::thread t([io = io_]() { io->run_one(); });

    io_->run_one();
    t.join();

    io_->reset();
    EXPECT_OUTCOME_TRUE_1(scheduler_->returnResult(call_id1, {}));

    EXPECT_OUTCOME_TRUE_1(scheduler_->returnResult(call_id2, {}));

    io_->run_one();
    io_->run_one();
    ASSERT_TRUE(cb1_call and cb2_call);
  }

  auto newWorker(std::string name, std::shared_ptr<WorkerMock> worker) {
    EXPECT_CALL(*worker, getInfo)
        .WillRepeatedly(
            testing::Return(primitives::WorkerInfo{.hostname = name}));

    std::unique_ptr<WorkerHandle> worker_handle =
        std::make_unique<WorkerHandle>();
    worker_handle->worker = std::move(worker);
    worker_handle->info =
        WorkerInfo{.hostname = std::move(name),
                   .resources = WorkerResources{.physical_memory = 1ul << 20,
                                                .swap_memory = 0,
                                                .reserved_memory = 0,
                                                .cpus = 0,
                                                .gpus = {}}};
    return worker_handle;
  }

  class WorkersTest : public ::testing::Test {
   protected:
    void SetUp() override {
      io_ = std::make_shared<boost::asio::io_context>();

      kv_ = std::make_shared<InMemoryStorage>();

      estimator_ = std::make_shared<EstimatorMock>();

      EXPECT_OUTCOME_TRUE(scheduler, EstimateSchedulerImpl::newScheduler(io_, kv_, estimator_));

      scheduler_ = scheduler;

      for (size_t i = 0; i < 3; i++) {
        auto worker = std::make_shared<WorkerMock>();
        workers_.push_back(worker);
        scheduler_->newWorker(newWorker(std::to_string(i), worker));
      }

      selector_ = std::make_shared<SelectorMock>();

      EXPECT_CALL(*selector_, is_satisfying(_, _, _))
          .WillRepeatedly(testing::Return(outcome::success(true)));

      EXPECT_CALL(*selector_, is_preferred(_, _, _))
          .WillRepeatedly(testing::Invoke(
              [](auto, auto &lhs, auto &rhs) { return lhs < rhs; }));
    }

    void TearDown() override {
      io_->stop();
    }

    std::vector<std::shared_ptr<WorkerMock>> workers_;

    std::shared_ptr<InMemoryStorage> kv_;
    std::shared_ptr<boost::asio::io_context> io_;
    std::shared_ptr<SelectorMock> selector_;
    std::shared_ptr<EstimatorMock> estimator_;
    std::shared_ptr<Scheduler> scheduler_;
  };

  /**
   * 3 workers with wid: 0, 1, 2
   * Selector sorts by ids
   * All workers, don't have any time data
   *
   * Worker 0 should be chosen
   */
  TEST_F(WorkersTest, WithoutTime) {
    SectorRef sector{.id = SectorId{
                         .miner = 42,
                         .sector = 1,
                     }};

    EXPECT_CALL(*estimator_, getTime(_, _))
        .WillRepeatedly(testing::Return(boost::none));

    auto prepare = [&](auto &worker) -> outcome::result<CallId> {
      EXPECT_OUTCOME_TRUE(info, worker->getInfo());
      if (info.hostname != "0") {
        return ERROR_TEXT("wrong worker was assigned");
      }

      return CallId{};
    };

    std::promise<outcome::result<CallResult>> promise_result;

    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(
        sector,
        primitives::kTTFinalize,
        selector_,
        prepare,
        [&](auto &worker) -> outcome::result<CallId> {
          return ERROR_TEXT("must not be called");
        },
        [&](const outcome::result<CallResult> &res) {
          FAIL() << "must not be called";
        },
        kDefaultTaskPriority,
        boost::none));

    io_->run_one();
  }

  /**
   * 3 workers with wid: 0, 1, 2
   * Selector sorts by ids
   * All workers has time data: 3, 2, 1 milliseconds respectively
   *
   * Worker 2 should be chosen
   */
  TEST_F(WorkersTest, WithTime) {
    auto task_type = primitives::kTTFinalize;

    SectorRef sector{.id = SectorId{
                         .miner = 42,
                         .sector = 1,
                     }};

    for (size_t i = 0; i < 3; i++) {
      EXPECT_CALL(*estimator_, getTime(i, task_type))
          .WillRepeatedly(testing::Return(boost::make_optional(double(3 - i))));
    }

    auto prepare = [&](auto &worker) -> outcome::result<CallId> {
      EXPECT_OUTCOME_TRUE(info, worker->getInfo());
      if (info.hostname != "2") {
        return ERROR_TEXT("wrong worker was assigned");
      }

      return CallId{};
    };

    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(
        sector,
        task_type,
        selector_,
        prepare,
        [](auto &worker) -> outcome::result<CallId> {
          return ERROR_TEXT("must not be called");
        },
        [](const outcome::result<CallResult> &res) {
          FAIL() << "must not be called";
        },
        kDefaultTaskPriority,
        boost::none));

    io_->run_one();
  }

  /**
   * 3 workers with wid: 0, 1, 2
   * Selector sorts by ids
   * Worker 0 has time data - 10 milliseconds
   *
   * Worker 1 should be chosen
   */
  TEST_F(WorkersTest, Mixed) {
    auto task_type = primitives::kTTFinalize;

    SectorRef sector{.id = SectorId{
                         .miner = 42,
                         .sector = 1,
                     }};

    EXPECT_CALL(*estimator_, getTime(_, _))
        .WillRepeatedly(testing::Return(boost::none));

    EXPECT_CALL(*estimator_, getTime(0, task_type))
        .WillRepeatedly(testing::Return(boost::make_optional(10.0)));

    auto prepare = [&](auto &worker) -> outcome::result<CallId> {
      EXPECT_OUTCOME_TRUE(info, worker->getInfo());
      if (info.hostname != "1") {
        return ERROR_TEXT("wrong worker was assigned");
      }

      return CallId{};
    };

    EXPECT_OUTCOME_TRUE_1(scheduler_->schedule(
        sector,
        task_type,
        selector_,
        prepare,
        [](auto &worker) -> outcome::result<CallId> {
          return ERROR_TEXT("must not be called");
        },
        [](const outcome::result<CallResult> &res) {
          FAIL() << "must not be called";
        },
        kDefaultTaskPriority,
        boost::none));

    io_->run_one();
  }
}  // namespace fc::sector_storage
