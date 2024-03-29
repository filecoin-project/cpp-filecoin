/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/manager_impl.hpp"

#include <boost/filesystem.hpp>
#include <gsl/span>
#include <sector_storage/stores/store_error.hpp>
#include "sector_storage/scheduler_utils.hpp"

#include "testutil/default_print.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/proofs/proof_engine_mock.hpp"
#include "testutil/mocks/sector_storage/scheduler_mock.hpp"
#include "testutil/mocks/sector_storage/stores/local_storage_mock.hpp"
#include "testutil/mocks/sector_storage/stores/local_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/remote_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/mocks/sector_storage/worker_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::sector_storage {
  namespace fs = boost::filesystem;

  using primitives::StoragePath;
  using primitives::piece::PaddedPieceSize;
  using primitives::sector::SectorInfo;
  using primitives::sector::toSectorInfo;
  using stores::AcquireSectorResponse;
  using stores::LocalStorageMock;
  using stores::LocalStoreMock;
  using stores::RemoteStoreMock;
  using stores::SectorIndexMock;
  using stores::SectorPaths;
  using ::testing::_;
  using ::testing::Eq;

  class ManagerTest : public test::BaseFS_Test {
   public:
    ManagerTest() : test::BaseFS_Test("fc_manager_test") {
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;
      sector_id_ = {
          .miner = 42,
          .sector = 1,
      };
      sector_ = {
          .id = sector_id_,
          .proof_type = seal_proof_type_,
      };

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

      EXPECT_CALL(*scheduler_, doNewWorker(_))
          .WillRepeatedly(::testing::Return());

      worker_ = std::make_shared<WorkerMock>();

      proof_engine_ = std::make_shared<proofs::ProofEngineMock>();

      EXPECT_CALL(*proof_engine_, getGPUDevices())
          .WillRepeatedly(
              testing::Return(outcome::success(std::vector<std::string>())));

      io_ = std::make_shared<boost::asio::io_context>();

      auto maybe_manager =
          ManagerImpl::newManager(io_,
                                  remote_store_,
                                  scheduler_,
                                  SealerConfig{
                                      .allow_precommit_1 = true,
                                      .allow_precommit_2 = true,
                                      .allow_commit = true,
                                      .allow_unseal = true,
                                  },
                                  proof_engine_);
      if (maybe_manager.has_value()) {
        manager_ = std::move(maybe_manager.value());
      } else {
        std::cerr << maybe_manager.error().message() << std::endl;
      }
    }

   protected:
    /**
     * Synchronously calls and waits for result of asynchronous Manager methods.
     *
     * @tparam F - method to call
     * @tparam Result - method result. Actually the parameter of the callback
     * handler in method.
     * @tparam Args - arguments to the method. All parameters before callback.
     */
    template <auto F, typename Result, typename... Args>
    outcome::result<Result> syncCall(Args... args) {
      std::promise<outcome::result<Result>> waiter;

      (*manager_.*F)(
          std::forward<Args>(args)...,
          [&waiter](const outcome::result<Result> &res) -> void {
            waiter.set_value(res);
          },
          kDefaultTaskPriority);

      return waiter.get_future().get();
    }

    RegisteredSealProof seal_proof_type_;

    std::shared_ptr<boost::asio::io_context> io_;

    std::shared_ptr<proofs::ProofEngineMock> proof_engine_;
    std::shared_ptr<WorkerMock> worker_;
    std::shared_ptr<SectorIndexMock> sector_index_;
    std::shared_ptr<LocalStorageMock> local_storage_;
    std::shared_ptr<LocalStoreMock> local_store_;
    std::shared_ptr<RemoteStoreMock> remote_store_;
    std::shared_ptr<SchedulerMock> scheduler_;
    std::shared_ptr<Manager> manager_;

    /** Arbitrary sector id */
    SectorId sector_id_;
    /** Arbitrary sector */
    SectorRef sector_;
  };

  /**
   * @given manager, sector
   * @when when try to seal precommit 1
   * @then success
   */
  TEST_F(ManagerTest, SealPreCommit1) {
    SealRandomness randomness({1, 2, 3});
    std::vector<PieceInfo> pieces = {
        PieceInfo{
            .size = PaddedPieceSize(128),
            .cid = "010001020001"_cid,
        },
    };

    EXPECT_CALL(
        *sector_index_,
        storageLock(sector_.id,
                    SectorFileType::FTUnsealed,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache)))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));

    std::vector<uint8_t> expected_result = {1, 2, 3, 4, 5};
    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(*worker_, sealPreCommit1(sector_, randomness, pieces))
        .WillOnce(testing::Return(outcome::success(call_id)));

    EXPECT_OUTCOME_TRUE(
        work_id,
        getWorkId(primitives::kTTPreCommit1,
                  std::make_tuple(sector_, randomness, pieces)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTPreCommit1,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(work_id)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id);
              cb(CallResult{expected_result, {}});
              return outcome::success();
            }));

    auto result = syncCall<&Manager::sealPreCommit1, PreCommit1Output>(
        sector_, randomness, pieces);
    EXPECT_OUTCOME_EQ(result, expected_result);
  }

  /**
   * @given manager, sector, output precommit1
   * @when when try to seal precommit 2
   * @then success
   */
  TEST_F(ManagerTest, SealPreCommit2) {
    std::vector<uint8_t> pre_commit_1_output = {1, 2, 3, 4, 5};

    EXPECT_CALL(
        *sector_index_,
        storageLock(
            sector_.id, SectorFileType::FTSealed, SectorFileType::FTCache))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));

    proofs::SealedAndUnsealedCID result_cids{
        .sealed_cid = "010001020001"_cid,
        .unsealed_cid = "010001020002"_cid,
    };

    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(*worker_, sealPreCommit2(sector_, pre_commit_1_output))
        .WillOnce(testing::Return(outcome::success(call_id)));

    EXPECT_OUTCOME_TRUE(
        work_id,
        getWorkId(primitives::kTTPreCommit2,
                  std::make_tuple(sector_, pre_commit_1_output)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTPreCommit2,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(work_id)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id);
              cb(CallResult{result_cids, {}});
              return outcome::success();
            }));

    auto result = syncCall<&Manager::sealPreCommit2, SectorCids>(
        sector_, pre_commit_1_output);
    EXPECT_OUTCOME_EQ(result, result_cids);
  }

  /**
   * @given manager, sector, output precommit2
   * @when when try to seal commit 1
   * @then success
   */
  TEST_F(ManagerTest, SealCommit1) {
    SealRandomness ticket({1, 2, 3});
    InteractiveRandomness seed({4, 5, 6});

    std::vector<PieceInfo> pieces = {PieceInfo{
        .size = PaddedPieceSize(128),
        .cid = "010001020003"_cid,
    }};

    proofs::SealedAndUnsealedCID cids{
        .sealed_cid = "010001020001"_cid,
        .unsealed_cid = "010001020002"_cid,
    };

    EXPECT_CALL(
        *sector_index_,
        storageLock(
            sector_.id, SectorFileType::FTSealed, SectorFileType::FTCache))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));

    std::vector<uint8_t> expected_result = {1, 2, 3, 4, 5};

    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(*worker_, sealCommit1(sector_, ticket, seed, pieces, cids))
        .WillOnce(testing::Return(outcome::success(call_id)));

    EXPECT_OUTCOME_TRUE(
        work_id,
        getWorkId(primitives::kTTCommit1,
                  std::make_tuple(sector_, ticket, seed, pieces, cids)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTCommit1,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(work_id)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              OUTCOME_TRY(work(worker_));
              cb(CallResult{expected_result, {}});
              return outcome::success();
            }));

    auto result = syncCall<&Manager::sealCommit1, Commit1Output>(
        sector_, ticket, seed, pieces, cids);
    EXPECT_OUTCOME_EQ(result, expected_result);
  }

  /**
   * @given manager, sector, output commit1
   * @when when try to seal commit 2
   * @then success
   */
  TEST_F(ManagerTest, SealCommit2) {
    std::vector<uint8_t> commit_1_output = {1, 2, 3, 4, 5};

    std::vector<uint8_t> expected_result = {1, 2, 3, 4, 5};
    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(*worker_, sealCommit2(sector_, commit_1_output))
        .WillOnce(testing::Return(outcome::success(call_id)));

    EXPECT_OUTCOME_TRUE(work_id,
                        getWorkId(primitives::kTTCommit2,
                                  std::make_tuple(sector_, commit_1_output)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTCommit2,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(work_id)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id);
              cb(CallResult{expected_result, {}});
              return outcome::success();
            }));

    auto result =
        syncCall<&Manager::sealCommit2, Proof>(sector_, commit_1_output);
    EXPECT_OUTCOME_EQ(result, expected_result);
  }

  /**
   * @given manager, sector
   * @when when try to finalize
   * @then success
   */
  TEST_F(ManagerTest, FinalizeSector) {
    std::vector<Range> keep_unsealed = {Range{
        .offset = UnpaddedPieceSize(127),
        .size = UnpaddedPieceSize(127),
    }};

    EXPECT_CALL(
        *sector_index_,
        storageLock(sector_.id,
                    SectorFileType::FTNone,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache
                                                | SectorFileType::FTUnsealed)))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));

    EXPECT_CALL(*sector_index_,
                storageFindSector(sector_.id, SectorFileType::FTUnsealed, _))
        .WillOnce(testing::Return(
            outcome::success(std::vector<stores::SectorStorageInfo>())));
    EXPECT_CALL(*sector_index_,
                storageFindSector(sector_.id, SectorFileType::FTSealed, _))
        .WillOnce(testing::Return(
            outcome::success(std::vector<stores::SectorStorageInfo>())));

    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(*worker_, finalizeSector(sector_, keep_unsealed))
        .WillOnce(testing::Return(outcome::success(call_id)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTFinalize,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(boost::none)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id);
              CallResult res;
              cb(res);
              return outcome::success();
            }));

    CallId call_id2{.sector = sector_.id, .id = "some UUID2"};
    EXPECT_CALL(
        *worker_,
        moveStorage(sector_,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache)))
        .WillOnce(testing::Return(outcome::success(call_id2)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTFetch,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(boost::none)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id2);
              CallResult res;
              cb(res);
              return outcome::success();
            }));

    std::ignore =
        syncCall<&Manager::finalizeSector, void>(sector_, keep_unsealed);
  }

  /**
   * @given manager, sector, pieces
   * @when try replica update
   * @then success, task is scheduled
   */
  TEST_F(ManagerTest, ReplicaUpdate) {
    std::vector<PieceInfo> pieces = {PieceInfo{
        .size = PaddedPieceSize(128),
        .cid = "010001020003"_cid,
    }};

    EXPECT_OUTCOME_TRUE(work_id,
                        getWorkId(primitives::kTTReplicaUpdate,
                                  std::make_tuple(sector_, pieces)));

    EXPECT_CALL(
        *sector_index_,
        storageLock(sector_.id,
                    SectorFileType::FTUnsealed | SectorFileType::FTSealed
                        | SectorFileType::FTCache,
                    SectorFileType::FTUpdate | SectorFileType::FTUpdateCache))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));

    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(*worker_, replicaUpdate(sector_, pieces))
        .WillOnce(testing::Return(outcome::success(call_id)));

    ReplicaUpdateOut expected_result{};

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTReplicaUpdate,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(work_id)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id);
              cb(CallResult{expected_result, {}});
              return outcome::success();
            }));

    auto result =
        syncCall<&Manager::replicaUpdate, ReplicaUpdateOut>(sector_, pieces);
    EXPECT_OUTCOME_EQ(result, expected_result);
  }

  /**
   * @given manager, sector, sector keys
   * @when try prove replica update1
   * @then success, task is scheduled
   */
  TEST_F(ManagerTest, ProveReplicaUpdate1) {
    CID sector_key_cid = "010001020001"_cid;
    CID new_sealed = "010001020002"_cid;
    CID new_unsealed = "010001020003"_cid;

    EXPECT_OUTCOME_TRUE(
        work_id,
        getWorkId(primitives::kTTProveReplicaUpdate1,
                  std::make_tuple(
                      sector_, sector_key_cid, new_sealed, new_unsealed)));

    EXPECT_CALL(*sector_index_,
                storageLock(sector_.id,
                            SectorFileType::FTSealed | SectorFileType::FTUpdate
                                | SectorFileType::FTCache
                                | SectorFileType::FTUpdateCache,
                            SectorFileType::FTNone))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));

    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(
        *worker_,
        proveReplicaUpdate1(sector_, sector_key_cid, new_sealed, new_unsealed))
        .WillOnce(testing::Return(outcome::success(call_id)));

    ReplicaVanillaProofs expected_result{};

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTProveReplicaUpdate1,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(work_id)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id);
              cb(CallResult{expected_result, {}});
              return outcome::success();
            }));

    auto result = syncCall<&Manager::proveReplicaUpdate1, ReplicaVanillaProofs>(
        sector_, sector_key_cid, new_sealed, new_unsealed);
    EXPECT_OUTCOME_EQ(result, expected_result);
  }

  /**
   * @given manager, sector, sector keys
   * @when try prove replica update2
   * @then success, task is scheduled
   */
  TEST_F(ManagerTest, ProveReplicaUpdate2) {
    CID sector_key_cid = "010001020001"_cid;
    CID new_sealed = "010001020002"_cid;
    CID new_unsealed = "010001020003"_cid;
    Update1Output update1_output{};

    EXPECT_OUTCOME_TRUE(work_id,
                        getWorkId(primitives::kTTProveReplicaUpdate2,
                                  std::make_tuple(sector_,
                                                  sector_key_cid,
                                                  new_sealed,
                                                  new_unsealed,
                                                  update1_output)));

    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(
        *worker_,
        proveReplicaUpdate2(
            sector_, sector_key_cid, new_sealed, new_unsealed, update1_output))
        .WillOnce(testing::Return(outcome::success(call_id)));

    ReplicaUpdateProof expected_result{};

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTProveReplicaUpdate2,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(work_id)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id);
              cb(CallResult{expected_result, {}});
              return outcome::success();
            }));

    auto result = syncCall<&Manager::proveReplicaUpdate2, ReplicaUpdateProof>(
        sector_, sector_key_cid, new_sealed, new_unsealed, update1_output);
    EXPECT_OUTCOME_EQ(result, expected_result);
  }

  /**
   * @given manager, sector
   * @when when try to remove
   * @then success
   */
  TEST_F(ManagerTest, Remove) {
    EXPECT_CALL(
        *sector_index_,
        storageLock(sector_.id,
                    SectorFileType::FTNone,
                    SectorFileType::FTCache | SectorFileType::FTSealed
                        | SectorFileType::FTUnsealed | SectorFileType::FTUpdate
                        | SectorFileType::FTUpdateCache))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));

    EXPECT_CALL(*remote_store_, remove(sector_.id, SectorFileType::FTCache))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*remote_store_, remove(sector_.id, SectorFileType::FTSealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*remote_store_, remove(sector_.id, SectorFileType::FTUnsealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*remote_store_, remove(sector_.id, SectorFileType::FTUpdate))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*remote_store_,
                remove(sector_.id, SectorFileType::FTUpdateCache))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(manager_->remove(sector_));
  }

  /**
   * @given manager, sector, piece
   * @when when try to add piece
   * @then success
   */
  TEST_F(ManagerTest, AddPiece) {
    UnpaddedPieceSize piece_size(127);

    EXPECT_CALL(
        *sector_index_,
        storageLock(
            sector_.id, SectorFileType::FTNone, SectorFileType::FTUnsealed))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));

    PieceInfo result{
        .size = PaddedPieceSize(128),
        .cid = "010001020001"_cid,
    };

    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(*worker_,
                doAddPiece(sector_, testing::IsEmpty(), piece_size, _))
        .WillOnce(testing::Return(outcome::success(call_id)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTAddPiece,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(boost::none)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id);
              cb(CallResult{result, {}});
              return outcome::success();
            }));

    EXPECT_OUTCOME_EQ(manager_->addPieceSync(sector_,
                                             {},
                                             piece_size,
                                             PieceData("/dev/random"),
                                             kDefaultTaskPriority),
                      result);
  }

  /**
   * @given manager, sector, piece
   * @when when try to read piece
   * @then pieces are equal
   */
  TEST_F(ManagerTest, ReadPiece) {
    UnpaddedByteIndex offset(127);
    UnpaddedPieceSize piece_size(127);
    CID cid = "010001020001"_cid;

    SealRandomness randomness({1, 2, 3, 4, 5});

    EXPECT_CALL(
        *sector_index_,
        storageLock(sector_.id,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTUnsealed))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));
    EXPECT_CALL(*sector_index_,
                storageFindSector(sector_.id, SectorFileType::FTUnsealed, _))
        .WillOnce(testing::Return(
            outcome::success(std::vector<stores::SectorStorageInfo>())));

    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(*worker_,
                unsealPiece(sector_, offset, piece_size, randomness, cid))
        .WillOnce(testing::Return(outcome::success(call_id)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTUnseal,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(boost::none)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id);
              CallResult res;
              cb(res);
              return outcome::success();
            }));

    CallId call_id2{.sector = sector_.id, .id = "some UUID2"};
    EXPECT_CALL(*worker_, doReadPiece(_, sector_, offset, piece_size))
        .WillOnce(testing::Return(outcome::success(call_id2)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTReadUnsealed,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(boost::none)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id2);
              cb(CallResult{true, {}});
              return outcome::success();
            }));

    int fd = -1;

    auto result = syncCall<&Manager::readPiece, bool>(
        PieceData(fd), sector_, offset, piece_size, randomness, cid);
    EXPECT_OUTCOME_EQ(result, true);
  }

  /**
   * @given manager, sector, piece
   * @when when try to add piece, but it is failed
   * @then ManagerErrors::kCannotReadData occurs
   */
  TEST_F(ManagerTest, ReadPieceFailed) {
    UnpaddedByteIndex offset(127);
    UnpaddedPieceSize piece_size(127);
    CID cid = "010001020001"_cid;

    SealRandomness randomness({1, 2, 3, 4, 5});

    EXPECT_CALL(
        *sector_index_,
        storageLock(sector_.id,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTUnsealed))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));
    EXPECT_CALL(*sector_index_,
                storageFindSector(sector_.id, SectorFileType::FTUnsealed, _))
        .WillOnce(testing::Return(
            outcome::success(std::vector<stores::SectorStorageInfo>())));

    CallId call_id{.sector = sector_.id, .id = "some UUID"};
    EXPECT_CALL(*worker_,
                unsealPiece(sector_, offset, piece_size, randomness, cid))
        .WillOnce(testing::Return(outcome::success(call_id)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTUnseal,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(boost::none)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id);
              CallResult res;
              cb(res);
              return outcome::success();
            }));

    CallId call_id2{.sector = sector_.id, .id = "some UUID2"};
    EXPECT_CALL(*worker_, doReadPiece(_, sector_, offset, piece_size))
        .WillOnce(testing::Return(outcome::success(call_id2)));

    EXPECT_CALL(*scheduler_,
                schedule(sector_,
                         primitives::kTTReadUnsealed,
                         _,
                         _,
                         _,
                         _,
                         kDefaultTaskPriority,
                         Eq(boost::none)))
        .WillOnce(testing::Invoke(
            [&](const SectorRef &sector,
                const TaskType &task_type,
                const std::shared_ptr<WorkerSelector> &selector,
                const WorkerAction &prepare,
                const WorkerAction &work,
                const ReturnCb &cb,
                uint64_t priority,
                const boost::optional<WorkId> &) -> outcome::result<void> {
              EXPECT_OUTCOME_EQ(work(worker_), call_id2);
              cb(CallResult{false, {}});
              return outcome::success();
            }));

    int fd = -1;

    auto result = syncCall<&Manager::readPiece, bool>(
        PieceData(fd), sector_, offset, piece_size, randomness, cid);
    EXPECT_OUTCOME_EQ(result, false);
  }

  /**
   * @given manager
   * @when when try to get fs stat
   * @then success
   */
  TEST_F(ManagerTest, GetFsStat) {
    StorageID storage = "storage-id";

    FsStat stat{
        .capacity = 300,
        .available = 100,
        .reserved = 200,
    };

    EXPECT_CALL(*local_store_, getFsStat(storage))
        .WillOnce(testing::Return(outcome::success(stat)));

    EXPECT_OUTCOME_EQ(manager_->getFsStat(storage), stat);
  }

  /**
   * @given manager, proofs files
   * @when when try to check provable
   * @then success
   */
  TEST_F(ManagerTest, CheckProvable) {
    EXPECT_OUTCOME_TRUE(ssize,
                        primitives::sector::getSectorSize(seal_proof_type_));
    auto sealed_path = base_path / toString(SectorFileType::FTSealed);
    auto cache_path = base_path / toString(SectorFileType::FTCache);

    ASSERT_TRUE(fs::create_directories(sealed_path));
    ASSERT_TRUE(fs::create_directories(cache_path));

    SectorId cannot_lock_sector_id{
        .miner = 42,
        .sector = 1,
    };

    SectorRef cannot_lock_sector{
        .id = cannot_lock_sector_id,
        .proof_type = seal_proof_type_,
    };

    EXPECT_CALL(
        *sector_index_,
        storageTryLock(cannot_lock_sector.id,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(testing::ByMove(nullptr)));

    SectorId non_exist_sector_id{
        .miner = 42,
        .sector = 2,
    };
    SectorRef non_exist_sector{
        .id = non_exist_sector_id,
        .proof_type = seal_proof_type_,
    };
    EXPECT_CALL(
        *sector_index_,
        storageTryLock(non_exist_sector.id,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    EXPECT_CALL(
        *local_store_,
        acquireSector(non_exist_sector,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(
            outcome::failure(stores::StoreError::kNotFoundSector)));

    SectorId non_exist_path_sector_id{
        .miner = 42,
        .sector = 3,
    };

    SectorRef non_exist_path_sector{.id = non_exist_path_sector_id,
                                    .proof_type = seal_proof_type_};
    EXPECT_CALL(
        *sector_index_,
        storageTryLock(non_exist_path_sector.id,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    SectorPaths non_exist_path_paths{
        .id = non_exist_path_sector.id,
        .unsealed = "",
        .sealed =
            (sealed_path
             / primitives::sector_file::sectorName(non_exist_path_sector.id))
                .string(),
        .cache =
            (cache_path
             / primitives::sector_file::sectorName(non_exist_path_sector.id))
                .string(),
    };

    AcquireSectorResponse non_exist_path_response{
        .paths = non_exist_path_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *local_store_,
        acquireSector(non_exist_path_sector,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(outcome::success(non_exist_path_response)));

    SectorId wrong_size_sector_id{
        .miner = 42,
        .sector = 4,
    };
    SectorRef wrong_size_sector{
        .id = wrong_size_sector_id,
        .proof_type = seal_proof_type_,
    };
    EXPECT_CALL(
        *sector_index_,
        storageTryLock(wrong_size_sector.id,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    SectorPaths wrong_size_paths{
        .id = wrong_size_sector.id,
        .unsealed = "",
        .sealed = (sealed_path
                   / primitives::sector_file::sectorName(wrong_size_sector.id))
                      .string(),
        .cache = (cache_path
                  / primitives::sector_file::sectorName(wrong_size_sector.id))
                     .string(),
    };

    AcquireSectorResponse wrong_size_response{
        .paths = wrong_size_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *local_store_,
        acquireSector(wrong_size_sector,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(outcome::success(wrong_size_response)));

    ASSERT_TRUE(fs::create_directories(wrong_size_paths.cache));
    {
      std::ofstream file(wrong_size_paths.sealed);
      ASSERT_TRUE(file.good());
      file.close();

      std::ofstream t_aux(
          (fs::path(wrong_size_paths.cache) / "t_aux").string());
      ASSERT_TRUE(t_aux.good());
      t_aux.close();
      std::ofstream p_aux(
          (fs::path(wrong_size_paths.cache) / "p_aux").string());
      ASSERT_TRUE(p_aux.good());
      p_aux.close();
      std::ofstream data_tree(
          (fs::path(wrong_size_paths.cache) / "sc-02-data-tree-r-last.dat")
              .string());
      ASSERT_TRUE(data_tree.good());
      data_tree.close();
    }

    SectorId success_sector_id{
        .miner = 42,
        .sector = 5,
    };

    SectorRef success_sector{
        .id = success_sector_id,
        .proof_type = seal_proof_type_,
    };

    SectorPaths success_paths{
        .id = success_sector.id,
        .unsealed = "",
        .sealed = (sealed_path
                   / primitives::sector_file::sectorName(success_sector.id))
                      .string(),
        .cache = (cache_path
                  / primitives::sector_file::sectorName(success_sector.id))
                     .string(),
    };

    AcquireSectorResponse success_response{
        .paths = success_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *sector_index_,
        storageTryLock(success_sector.id,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    EXPECT_CALL(
        *local_store_,
        acquireSector(success_sector,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(outcome::success(success_response)));

    ASSERT_TRUE(fs::create_directories(success_paths.cache));
    {
      std::ofstream file(success_paths.sealed);
      ASSERT_TRUE(file.good());
      file.close();
      fs::resize_file(success_paths.sealed, ssize);

      std::ofstream t_aux((fs::path(success_paths.cache) / "t_aux").string());
      ASSERT_TRUE(t_aux.good());
      t_aux.close();
      std::ofstream p_aux((fs::path(success_paths.cache) / "p_aux").string());
      ASSERT_TRUE(p_aux.good());
      p_aux.close();
      std::ofstream data_tree(
          (fs::path(success_paths.cache) / "sc-02-data-tree-r-last.dat")
              .string());
      ASSERT_TRUE(data_tree.good());
      data_tree.close();
    }

    std::vector<SectorRef> sectors = {success_sector,
                                      wrong_size_sector,
                                      non_exist_path_sector,
                                      non_exist_sector,
                                      cannot_lock_sector};

    EXPECT_OUTCOME_TRUE(post_proof,
                        getRegisteredWindowPoStProof(seal_proof_type_));

    EXPECT_OUTCOME_TRUE(bad_sectors,
                        manager_->checkProvable(post_proof, sectors));
    EXPECT_THAT(bad_sectors,
                testing::UnorderedElementsAre(wrong_size_sector.id,
                                              non_exist_path_sector.id,
                                              non_exist_sector.id,
                                              cannot_lock_sector.id));
  }

  /**
   * @given manager
   * @when when try to generate winning post
   * @then success
   */
  TEST_F(ManagerTest, GenerateWinningPoSt) {
    EXPECT_OUTCOME_TRUE(
        post_proof,
        primitives::sector::getRegisteredWinningPoStProof(seal_proof_type_));

    EXPECT_OUTCOME_TRUE(randomness,
                        PoStRandomness::fromString(std::string(32, '\xff')));

    std::vector<proofs::PoStProof> result = {{
        .registered_proof = post_proof,
        .proof = {1, 2, 3, 4, 5},
    }};

    uint64_t miner_id = 42;

    std::vector<ExtendedSectorInfo> public_sectors = {
        ExtendedSectorInfo{.registered_proof = seal_proof_type_,
                           .sector = 1,
                           .sealed_cid = "010001020001"_cid},
    };

    SectorId success_sector_id{
        .miner = miner_id,
        .sector = public_sectors[0].sector,
    };

    SectorRef success_sector{
        .id = success_sector_id,
        .proof_type = seal_proof_type_,
    };

    SectorPaths success_paths{
        .id = success_sector.id,
        .unsealed = "",
        .sealed = (base_path / toString(SectorFileType::FTSealed)
                   / primitives::sector_file::sectorName(success_sector.id))
                      .string(),
        .cache = (base_path / toString(SectorFileType::FTCache)
                  / primitives::sector_file::sectorName(success_sector.id))
                     .string(),
    };

    AcquireSectorResponse success_response{
        .paths = success_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *local_store_,
        acquireSector(success_sector,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(outcome::success(success_response)));

    EXPECT_CALL(
        *sector_index_,
        storageTryLock(success_sector.id,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    std::vector<proofs::PrivateSectorInfo> private_sector = {
        proofs::PrivateSectorInfo{.info = toSectorInfo(public_sectors[0]),
                                  .cache_dir_path = success_paths.cache,
                                  .post_proof_type = post_proof,
                                  .sealed_sector_path = success_paths.sealed}};

    auto s = proofs::newSortedPrivateSectorInfo(private_sector);

    EXPECT_CALL(*proof_engine_, generateWinningPoSt(miner_id, s, randomness))
        .WillOnce(testing::Return(outcome::success(result)));

    EXPECT_OUTCOME_EQ(
        manager_->generateWinningPoSt(miner_id, public_sectors, randomness),
        result)
  }

  /**
   * @given manager
   * @when when try to generate winning post with skip sectors
   * @then ManagerErrors::kSomeSectorSkipped occurs
   */
  TEST_F(ManagerTest, GenerateWinningPoStWithSkip) {
    EXPECT_OUTCOME_TRUE(
        post_proof,
        primitives::sector::getRegisteredWinningPoStProof(seal_proof_type_));

    EXPECT_OUTCOME_TRUE(randomness,
                        PoStRandomness::fromString(std::string(32, '\xff')));

    std::vector<proofs::PoStProof> result = {{
        .registered_proof = post_proof,
        .proof = {1, 2, 3, 4, 5},
    }};

    uint64_t miner_id = 42;

    std::vector<ExtendedSectorInfo> public_sectors = {
        ExtendedSectorInfo{.registered_proof = seal_proof_type_,
                           .sector = 1,
                           .sealed_cid = "010001020001"_cid},
        ExtendedSectorInfo{.registered_proof = seal_proof_type_,
                           .sector = 2,
                           .sealed_cid = "010001020002"_cid},
    };

    EXPECT_CALL(*sector_index_,
                storageTryLock(
                    SectorId{
                        .miner = miner_id,
                        .sector = public_sectors[1].sector,
                    },
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTNone))
        .WillOnce(testing::Return(testing::ByMove(nullptr)));

    SectorId success_sector_id{
        .miner = miner_id,
        .sector = public_sectors[0].sector,
    };

    SectorRef success_sector{
        .id = success_sector_id,
        .proof_type = seal_proof_type_,
    };

    SectorPaths success_paths{
        .id = success_sector.id,
        .unsealed = "",
        .sealed = (base_path / toString(SectorFileType::FTSealed)
                   / primitives::sector_file::sectorName(success_sector.id))
                      .string(),
        .cache = (base_path / toString(SectorFileType::FTCache)
                  / primitives::sector_file::sectorName(success_sector.id))
                     .string(),
    };

    AcquireSectorResponse success_response{
        .paths = success_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *local_store_,
        acquireSector(success_sector,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(outcome::success(success_response)));

    EXPECT_CALL(
        *sector_index_,
        storageTryLock(success_sector.id,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    EXPECT_OUTCOME_ERROR(
        ManagerErrors::kSomeSectorSkipped,
        manager_->generateWinningPoSt(miner_id, public_sectors, randomness))
  }

  /**
   * @given manager
   * @when when try to generate window post
   * @then success
   */
  TEST_F(ManagerTest, GenerateWindowPoSt) {
    EXPECT_OUTCOME_TRUE(
        post_proof,
        primitives::sector::getRegisteredWindowPoStProof(seal_proof_type_));

    EXPECT_OUTCOME_TRUE(randomness,
                        PoStRandomness::fromString(std::string(32, '\xff')));

    std::vector<proofs::PoStProof> proof = {{
        .registered_proof = post_proof,
        .proof = {1, 2, 3, 4, 5},
    }};

    uint64_t miner_id = 42;

    std::vector<ExtendedSectorInfo> public_sectors = {
        ExtendedSectorInfo{.registered_proof = seal_proof_type_,
                           .sector = 1,
                           .sealed_cid = "010001020001"_cid},
        ExtendedSectorInfo{.registered_proof = seal_proof_type_,
                           .sector = 2,
                           .sealed_cid = "010001020002"_cid},
    };

    std::vector<SectorId> skipped({
        SectorId{
            .miner = miner_id,
            .sector = public_sectors[1].sector,
        },
    });

    Prover::WindowPoStResponse result{
        .proof = proof,
        .skipped = skipped,
    };

    EXPECT_CALL(*sector_index_,
                storageTryLock(
                    SectorId{
                        .miner = miner_id,
                        .sector = public_sectors[1].sector,
                    },
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTNone))
        .WillOnce(testing::Return(testing::ByMove(nullptr)));

    SectorId success_sector_id{
        .miner = miner_id,
        .sector = public_sectors[0].sector,
    };

    SectorRef success_sector{
        .id = success_sector_id,
        .proof_type = seal_proof_type_,
    };

    SectorPaths success_paths{
        .id = success_sector.id,
        .unsealed = "",
        .sealed = (base_path / toString(SectorFileType::FTSealed)
                   / primitives::sector_file::sectorName(success_sector.id))
                      .string(),
        .cache = (base_path / toString(SectorFileType::FTCache)
                  / primitives::sector_file::sectorName(success_sector.id))
                     .string(),
    };

    AcquireSectorResponse success_response{
        .paths = success_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *local_store_,
        acquireSector(success_sector,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(outcome::success(success_response)));

    EXPECT_CALL(
        *sector_index_,
        storageTryLock(success_sector.id,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    std::vector<proofs::PrivateSectorInfo> private_sector = {
        proofs::PrivateSectorInfo{.info = toSectorInfo(public_sectors[0]),
                                  .cache_dir_path = success_paths.cache,
                                  .post_proof_type = post_proof,
                                  .sealed_sector_path = success_paths.sealed}};

    auto s = proofs::newSortedPrivateSectorInfo(private_sector);

    EXPECT_CALL(*proof_engine_, generateWindowPoSt(miner_id, s, randomness))
        .WillOnce(testing::Return(outcome::success(proof)));

    EXPECT_OUTCOME_EQ(
        manager_->generateWindowPoSt(miner_id, public_sectors, randomness),
        result)
  }  // namespace fc::sector_storage

  /**
   * @given absolute path
   * @when when try to add new local storage
   * @then opened this path
   */
  TEST_F(ManagerTest, addLocalStorageWithoutExpand) {
    std::string path = "/some/path/here";

    EXPECT_CALL(*local_store_, openPath(path))
        .WillOnce(::testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(manager_->addLocalStorage(path));
  }

  /**
   * @given path from home
   * @when when try to add new local storage
   * @then opened absolute path
   */
  TEST_F(ManagerTest, addLocalStorageWithExpand) {
    std::string path = "~/some/path/here";
    fs::path home(std::getenv("HOME"));
    std::string result = (home / "some/path/here").string();

    EXPECT_CALL(*local_store_, openPath(result))
        .WillOnce(::testing::Return(outcome::success()));

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
        .WillOnce(::testing::Return(outcome::success(paths)));

    EXPECT_OUTCOME_TRUE(storages, manager_->getLocalStorages());
    EXPECT_THAT(storages, ::testing::UnorderedElementsAreArray(result));
  }
}  // namespace fc::sector_storage
