/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/local_worker.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <random>

#include "api/storage_miner/storage_api.hpp"
#include "common/error_text.hpp"
#include "proofs/proofs_error.hpp"
#include "sector_storage/stores/store_error.hpp"
#include "testutil/default_print.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/mocks/proofs/proof_engine_mock.hpp"
#include "testutil/mocks/sector_storage/stores/local_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/remote_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::sector_storage {
  using primitives::ActorId;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::StoragePath;
  using primitives::piece::PaddedPieceSize;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::InteractiveRandomness;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::SectorRef;
  using primitives::sector_file::SectorFile;
  using primitives::sector_file::SectorFileType;
  using proofs::PieceInfo;
  using proofs::ProofEngineMock;
  using proofs::SealVerifyInfo;
  using proofs::Ticket;
  using proofs::UnsealedCID;
  using stores::AcquireMode;
  using stores::AcquireSectorResponse;
  using stores::LocalStoreMock;
  using stores::PathType;
  using stores::RemoteStoreMock;
  using stores::SectorIndexMock;
  using stores::SectorPaths;
  using testing::_;
  using testing::Eq;
  using testing::Ne;

  class LocalWorkerTest : public test::BaseFS_Test {
   public:
    LocalWorkerTest() : test::BaseFS_Test("fc_local_worker_test") {
      io_context_ = std::make_shared<boost::asio::io_context>();

      tasks_ = {
          primitives::kTTAddPiece,
          primitives::kTTPreCommit1,
          primitives::kTTPreCommit2,
      };
      sector_ = SectorRef{.id =
                              SectorId{
                                  .miner = 42,
                                  .sector = 1,
                              },
                          .proof_type = RegisteredSealProof::kStackedDrg2KiBV1};
      sector_size_ = getSectorSize(sector_.proof_type).value();

      hostname_ = "test_worker";
      config_ = WorkerConfig{.custom_hostname = hostname_,
                             .task_types = tasks_,
                             .is_no_swap = false};
      store_ = std::make_shared<RemoteStoreMock>();
      local_store_ = std::make_shared<LocalStoreMock>();
      sector_index_ = std::make_shared<SectorIndexMock>();
      proof_engine_ = std::make_shared<ProofEngineMock>();

      EXPECT_CALL(*store_, getLocalStore())
          .WillRepeatedly(testing::Return(local_store_));

      EXPECT_CALL(*store_, getSectorIndex())
          .WillRepeatedly(testing::Return(sector_index_));

      EXPECT_CALL(*local_store_, getSectorIndex())
          .WillRepeatedly(testing::Return(sector_index_));

      return_interface_ = std::make_shared<WorkerReturn>();

      local_worker_ = std::make_shared<LocalWorker>(
          io_context_, config_, return_interface_, store_, proof_engine_);
    }

   protected:
    std::shared_ptr<boost::asio::io_context> io_context_;

    SectorRef sector_;
    std::set<primitives::TaskType> tasks_;
    SectorSize sector_size_;
    WorkerConfig config_;

    std::string hostname_;

    std::shared_ptr<RemoteStoreMock> store_;
    std::shared_ptr<LocalStoreMock> local_store_;
    std::shared_ptr<SectorIndexMock> sector_index_;
    std::shared_ptr<ProofEngineMock> proof_engine_;
    std::shared_ptr<WorkerReturn> return_interface_;

    std::shared_ptr<LocalWorker> local_worker_;
  };

  /**
   * @given local worker
   * @when when try to getSupportedTask
   * @then gets supported tasks for the worker
   */
  TEST_F(LocalWorkerTest, getSupportedTask) {
    EXPECT_OUTCOME_EQ(local_worker_->getSupportedTask(), tasks_);
  }

  /**
   * @given local worker
   * @when when try to getInfo
   * @then gets info about the worker
   */
  TEST_F(LocalWorkerTest, getInfo) {
    std::vector<std::string> gpus = {"GPU1", "GPU2"};
    EXPECT_CALL(*proof_engine_, getGPUDevices())
        .WillOnce(testing::Return(gpus));

    EXPECT_OUTCOME_TRUE(info, local_worker_->getInfo());
    ASSERT_EQ(info.hostname, hostname_);
    ASSERT_EQ(info.resources.gpus, gpus);
  }

  /**
   * @given local worker
   * @when when try to getAccessiblePaths
   * @then gets paths that accessible by the worker
   */
  TEST_F(LocalWorkerTest, getAccessiblePaths) {
    std::vector<StoragePath> paths = {StoragePath{
                                          .id = "id1",
                                          .weight = 10,
                                          .local_path = "/some/path/1",
                                          .can_seal = false,
                                          .can_store = true,
                                      },
                                      StoragePath{
                                          .id = "id2",
                                          .weight = 100,
                                          .local_path = "/some/path/2",
                                          .can_seal = true,
                                          .can_store = false,
                                      }};
    EXPECT_CALL(*local_store_, getAccessiblePaths())
        .WillOnce(testing::Return(outcome::success(paths)));
    EXPECT_OUTCOME_EQ(local_worker_->getAccessiblePaths(), paths);
  }

  /**
   * @given sector
   * @when when try to preSealCommit with sum of sizes of pieces and not equal
   * to sector size
   * @then Error occurs
   */
  TEST_F(LocalWorkerTest, PreCommit_MatchSumError) {
    SealRandomness ticket{{5, 4, 2}};

    AcquireSectorResponse response{};

    response.paths.sealed = (base_path / toString(SectorFileType::FTSealed)
                             / primitives::sector_file::sectorName(sector_.id))
                                .string();
    response.paths.cache = (base_path / toString(SectorFileType::FTCache)
                            / primitives::sector_file::sectorName(sector_.id))
                               .string();

    EXPECT_CALL(*store_, remove(sector_.id, SectorFileType::FTSealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*store_, remove(sector_.id, SectorFileType::FTCache))
        .WillOnce(testing::Return(outcome::success()));

    bool is_storage_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        static_cast<SectorFileType>(SectorFileType::FTCache
                                                    | SectorFileType::FTSealed),
                        _,
                        PathType::kSealing))
        .WillOnce(testing::Return(
            outcome::success([&]() { is_storage_clear = true; })));

    EXPECT_CALL(*sector_index_,
                storageDeclareSector(response.storages.cache,
                                     sector_.id,
                                     SectorFileType::FTCache,
                                     false))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*sector_index_,
                storageDeclareSector(response.storages.sealed,
                                     sector_.id,
                                     SectorFileType::FTSealed,
                                     false))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(
        *store_,
        acquireSector(sector_,
                      SectorFileType::FTUnsealed,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      PathType::kSealing,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(response)));

    ASSERT_TRUE(boost::filesystem::create_directory(
        base_path / toString(SectorFileType::FTSealed)));
    ASSERT_TRUE(boost::filesystem::create_directory(
        base_path / toString(SectorFileType::FTCache)));

    std::vector<PieceInfo> pieces{
        {.size = PaddedPieceSize(1024), .cid = "010001020001"_cid}};

    MOCK_API(return_interface_, ReturnSealPreCommit1);

    EXPECT_OUTCOME_TRUE(call_id,
                        local_worker_->sealPreCommit1(sector_, ticket, pieces));

    CallError error;
    EXPECT_CALL(mock_ReturnSealPreCommit1, Call(call_id, _, Ne(boost::none)))
        .WillOnce(
            testing::Invoke([&](CallId call_id,
                                boost::optional<PreCommit1Output> maybe_value,
                                boost::optional<CallError> maybe_error)
                                -> outcome::result<void> {
              if (maybe_error.has_value()) {
                error = maybe_error.value();
                return outcome::success();
              }

              return ERROR_TEXT("ERROR: no error");
            }));

    io_context_->run_one();

    ASSERT_EQ(error.message,
              outcome::result<void>(WorkerErrors::kPiecesDoNotMatchSectorSize)
                  .error()
                  .message());

    ASSERT_TRUE(is_storage_clear);
  }

  /**
   * @given local worker
   * @when when try to finalize sector without keep unsealed
   * @then sector file is removed
   */
  TEST_F(LocalWorkerTest, FinalizeSectorWithoutKeepUnsealed) {
    SectorPaths paths{
        .id = sector_.id,
        .unsealed = "",
        .sealed = "",
        .cache = "some/cache/path",
    };

    SectorPaths storages{
        .id = sector_.id,
        .unsealed = "",
        .sealed = "",
        .cache = "cache-storage-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTCache,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(resp));

    bool is_clear = false;
    EXPECT_CALL(
        *local_store_,
        reserve(sector_, SectorFileType::FTNone, storages, PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(*proof_engine_, clearCache(sector_size_, paths.cache))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*store_, remove(sector_.id, SectorFileType::FTUnsealed))
        .WillOnce(testing::Return(outcome::success()));

    MOCK_API(return_interface_, ReturnFinalizeSector);
    EXPECT_OUTCOME_TRUE(call_id, local_worker_->finalizeSector(sector_, {}));

    EXPECT_CALL(mock_ReturnFinalizeSector, Call(call_id, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();

    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker
   * @when when try to finalize sector with keep unsealed
   * @then sector file is keeped
   */
  TEST_F(LocalWorkerTest, FinalizeSectorWithKeepUnsealed) {
    std::vector<Range> ranges = {
        Range{
            .offset = UnpaddedPieceSize(0),
            .size = UnpaddedPieceSize(127),
        },
    };

    auto file_path =
        base_path / primitives::sector_file::sectorName(sector_.id);
    PaddedPieceSize piece_size(256);
    PaddedPieceSize max_size(sector_size_);
    {
      EXPECT_OUTCOME_TRUE(file,
                          SectorFile::createFile(file_path.string(), max_size));

      EXPECT_OUTCOME_TRUE_1(file->write(
          PieceData("/dev/random"), PaddedPieceSize(0), piece_size));

      EXPECT_OUTCOME_EQ(file->hasAllocated(0, piece_size.unpadded()), true);
    }

    SectorPaths unsealed_paths{
        .id = sector_.id,
        .unsealed = file_path.string(),
        .sealed = "",
        .cache = "",
    };

    SectorPaths unsealed_storages{
        .id = sector_.id,
        .unsealed = "unsealed-storage-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unsealed_resp{
        .paths = unsealed_paths,
        .storages = unsealed_storages,
    };

    bool is_clear_unsealed = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        SectorFileType::FTNone,
                        unsealed_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear_unsealed = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(unsealed_resp));

    SectorPaths cache_paths{
        .id = sector_.id,
        .unsealed = "",
        .sealed = "",
        .cache = "some/cache/path",
    };

    SectorPaths cache_storages{
        .id = sector_.id,
        .unsealed = "",
        .sealed = "",
        .cache = "cache-storage-id",
    };

    AcquireSectorResponse cache_resp{
        .paths = cache_paths,
        .storages = cache_storages,
    };

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        SectorFileType::FTNone,
                        cache_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTCache,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(cache_resp));

    EXPECT_CALL(*proof_engine_, clearCache(sector_size_, cache_paths.cache))
        .WillOnce(testing::Return(outcome::success()));

    MOCK_API(return_interface_, ReturnFinalizeSector);
    EXPECT_OUTCOME_TRUE(call_id,
                        local_worker_->finalizeSector(sector_, ranges));

    EXPECT_CALL(mock_ReturnFinalizeSector, Call(call_id, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();

    ASSERT_TRUE(is_clear);
    ASSERT_TRUE(is_clear_unsealed);
    EXPECT_OUTCOME_TRUE(file,
                        SectorFile::openFile(file_path.string(), max_size));
    EXPECT_OUTCOME_EQ(file->hasAllocated(0, piece_size.unpadded()), false);
    EXPECT_OUTCOME_EQ(file->hasAllocated(0, ranges[0].size), true);
  }

  /**
   * @given local worker, sector
   * @when when try to seal pre commit 1
   * @then success
   */
  TEST_F(LocalWorkerTest, SealPreCommit1) {
    proofs::Ticket randomness{{1, 2, 3}};

    std::vector<PieceInfo> pieces = {PieceInfo{
                                         .size = PaddedPieceSize(1024),
                                         .cid = CID(),
                                     },
                                     PieceInfo{
                                         .size = PaddedPieceSize(1024),
                                         .cid = CID(),
                                     }};

    EXPECT_CALL(*store_, remove(sector_.id, SectorFileType::FTSealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*store_, remove(sector_.id, SectorFileType::FTCache))
        .WillOnce(testing::Return(outcome::success()));

    bool is_storage_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        static_cast<SectorFileType>(SectorFileType::FTCache
                                                    | SectorFileType::FTSealed),
                        _,
                        PathType::kSealing))
        .WillOnce(testing::Return(
            outcome::success([&]() { is_storage_clear = true; })));

    auto cache{base_path / toString(SectorFileType::FTCache)};

    ASSERT_TRUE(fs::create_directories(cache));

    SectorPaths paths{
        .id = sector_.id,
        .unsealed = "",
        .sealed = (base_path / primitives::sector_file::sectorName(sector_.id))
                      .string(),
        .cache =
            (cache / primitives::sector_file::sectorName(sector_.id)).string(),
    };

    SectorPaths storages{
        .id = sector_.id,
        .unsealed = "",
        .sealed = "sealed-id",
        .cache = "cache-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    EXPECT_CALL(
        *store_,
        acquireSector(sector_,
                      SectorFileType::FTUnsealed,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      PathType::kSealing,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(resp)));

    EXPECT_CALL(
        *sector_index_,
        storageDeclareSector(
            resp.storages.cache, sector_.id, SectorFileType::FTCache, false))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(
        *sector_index_,
        storageDeclareSector(
            resp.storages.sealed, sector_.id, SectorFileType::FTSealed, false))
        .WillOnce(testing::Return(outcome::success()));

    proofs::Phase1Output p1o = {0, 1, 2, 3};
    EXPECT_CALL(*proof_engine_,
                sealPreCommitPhase1(sector_.proof_type,
                                    resp.paths.cache,
                                    resp.paths.unsealed,
                                    resp.paths.sealed,
                                    sector_.id.sector,
                                    sector_.id.miner,
                                    randomness,
                                    gsl::make_span<const PieceInfo>(
                                        pieces.data(), pieces.size())))
        .WillOnce(testing::Return(outcome::success(p1o)));

    MOCK_API(return_interface_, ReturnSealPreCommit1);

    EXPECT_OUTCOME_TRUE(
        call_id, local_worker_->sealPreCommit1(sector_, randomness, pieces));

    EXPECT_CALL(mock_ReturnSealPreCommit1,
                Call(call_id, Eq(p1o), Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();

    ASSERT_TRUE(fs::exists(paths.sealed));
    ASSERT_TRUE(is_storage_clear);
  }

  /**
   * @given local worker, sector, precommit1 output
   * @when when try to seal pre commit 2
   * @then success
   */
  TEST_F(LocalWorkerTest, SealPreCommit2) {
    auto cache{base_path / toString(SectorFileType::FTCache)};

    ASSERT_TRUE(fs::create_directories(cache));

    SectorPaths paths{
        .id = sector_.id,
        .unsealed = "",
        .sealed = (base_path / primitives::sector_file::sectorName(sector_.id))
                      .string(),
        .cache =
            (cache / primitives::sector_file::sectorName(sector_.id)).string(),
    };

    SectorPaths storages{
        .id = sector_.id,
        .unsealed = "",
        .sealed = "sealed-id",
        .cache = "cache-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    bool is_clear = false;
    EXPECT_CALL(
        *local_store_,
        reserve(sector_, SectorFileType::FTNone, storages, PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(
        *store_,
        acquireSector(sector_,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kSealing,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(resp)));

    proofs::Phase1Output p1o = {0, 1, 2, 3};

    SectorCids cids{
        .sealed_cid = "010001020001"_cid,
        .unsealed_cid = "010001020002"_cid,
    };

    EXPECT_CALL(*proof_engine_,
                sealPreCommitPhase2(
                    gsl::make_span<const uint8_t>(p1o.data(), p1o.size()),
                    paths.cache,
                    paths.sealed))
        .WillOnce(testing::Return(outcome::success(cids)));

    MOCK_API(return_interface_, ReturnSealPreCommit2);

    EXPECT_OUTCOME_TRUE(
        call_id,
        local_worker_->sealPreCommit2(sector_, p1o));  // TODO: переделать

    EXPECT_CALL(mock_ReturnSealPreCommit2,
                Call(call_id, Eq(cids), Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));
    io_context_->run_one();

    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker, sector, precommit2 output
   * @when when try to seal commit 1
   * @then success
   */
  TEST_F(LocalWorkerTest, SealCommit1) {
    CID unsealed_cid = "010001020001"_cid;
    CID sealed_cid = "010001020002"_cid;
    SectorCids cids{
        .sealed_cid = sealed_cid,
        .unsealed_cid = unsealed_cid,
    };
    proofs::Ticket randomness{{1, 2, 3}};
    primitives::sector::InteractiveRandomness seed{{4, 5, 6}};
    std::vector<PieceInfo> pieces = {PieceInfo{
                                         .size = PaddedPieceSize(1024),
                                         .cid = CID(),
                                     },
                                     PieceInfo{
                                         .size = PaddedPieceSize(1024),
                                         .cid = CID(),
                                     }};

    auto cache{base_path / toString(SectorFileType::FTCache)};

    ASSERT_TRUE(fs::create_directories(cache));

    SectorPaths paths{
        .id = sector_.id,
        .unsealed = "",
        .sealed = (base_path / primitives::sector_file::sectorName(sector_.id))
                      .string(),
        .cache =
            (cache / primitives::sector_file::sectorName(sector_.id)).string(),
    };

    SectorPaths storages{
        .id = sector_.id,
        .unsealed = "",
        .sealed = "sealed-id",
        .cache = "cache-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    bool is_clear = false;
    EXPECT_CALL(
        *local_store_,
        reserve(sector_, SectorFileType::FTNone, storages, PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(
        *store_,
        acquireSector(sector_,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kSealing,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(resp)));

    Commit1Output c1o{{1, 2, 3}};
    EXPECT_CALL(*proof_engine_,
                sealCommitPhase1(sector_.proof_type,
                                 sealed_cid,
                                 unsealed_cid,
                                 paths.cache,
                                 paths.sealed,
                                 sector_.id.sector,
                                 sector_.id.miner,
                                 randomness,
                                 seed,
                                 gsl::make_span<const PieceInfo>(
                                     pieces.data(), pieces.size())))
        .WillOnce(testing::Return(outcome::success(c1o)));

    MOCK_API(return_interface_, ReturnSealCommit1);

    EXPECT_OUTCOME_TRUE(
        call_id,
        local_worker_->sealCommit1(sector_, randomness, seed, pieces, cids));

    EXPECT_CALL(mock_ReturnSealCommit1, Call(call_id, Eq(c1o), Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();

    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker, sector, commit1 output
   * @when when try to seal commit 2
   * @then success
   */
  TEST_F(LocalWorkerTest, SealCommit2) {
    Commit1Output c1o{{1, 2, 3}};
    Proof proof{{4, 5, 6}};

    EXPECT_CALL(
        *proof_engine_,
        sealCommitPhase2(gsl::make_span<const uint8_t>(c1o.data(), c1o.size()),
                         sector_.id.sector,
                         sector_.id.miner))
        .WillOnce(testing::Return(outcome::success(proof)));

    MOCK_API(return_interface_, ReturnSealCommit2);

    EXPECT_OUTCOME_TRUE(call_id, local_worker_->sealCommit2(sector_, c1o));

    EXPECT_CALL(mock_ReturnSealCommit2,
                Call(call_id, Eq(proof), Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();
  }

  /**
   * @given local worker, unsealed sector file
   * @when when try to unseal already unsealed
   * @then success, without unseal twice
   */
  TEST_F(LocalWorkerTest, UnsealPieceAlreadyUnsealed) {
    PaddedPieceSize max_size(sector_size_);

    auto offset{127};
    UnpaddedPieceSize piece_size(127);

    SealRandomness randomness{{1, 2, 3}};

    CID unsealed_cid = "010001020001"_cid;

    ASSERT_TRUE(fs::create_directories(base_path
                                       / toString(SectorFileType::FTUnsealed)));

    SectorPaths unseal_paths{
        .id = sector_.id,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector_.id))
                        .string(),
        .sealed = "",
        .cache = "",
    };

    {
      EXPECT_OUTCOME_TRUE(
          file, SectorFile::createFile(unseal_paths.unsealed, max_size));
      EXPECT_OUTCOME_TRUE_1(file->write(PieceData("/dev/random"),
                                        primitives::piece::paddedIndex(offset),
                                        piece_size.padded()));
    }

    SectorPaths unseal_storages{
        .id = sector_.id,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        SectorFileType::FTNone,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    MOCK_API(return_interface_, ReturnUnsealPiece);

    EXPECT_OUTCOME_TRUE(
        call_id,
        local_worker_->unsealPiece(
            sector_, offset, piece_size, randomness, unsealed_cid));

    EXPECT_CALL(mock_ReturnUnsealPiece, Call(call_id, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();
    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker, unsealed sector file(empty)
   * @when when try to unseal
   * @then file is unsealed
   */
  TEST_F(LocalWorkerTest, UnsealPieceAlreadyExistFile) {
    PaddedPieceSize max_size(sector_size_);

    SealRandomness randomness{{1, 2, 3}};

    CID unsealed_cid = "010001020001"_cid;

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      ASSERT_TRUE(fs::create_directories(base_path / toString(type)));
    }

    SectorPaths unseal_paths{
        .id = sector_.id,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector_.id))
                        .string(),
        .sealed = "",
        .cache = "",
    };

    {
      EXPECT_OUTCOME_TRUE_1(
          SectorFile::createFile(unseal_paths.unsealed, max_size));
    }

    SectorPaths unseal_storages{
        .id = sector_.id,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        SectorFileType::FTNone,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    SectorPaths paths{
        .id = sector_.id,
        .unsealed = "",
        .sealed = (base_path / toString(SectorFileType::FTSealed)
                   / primitives::sector_file::sectorName(sector_.id))
                      .string(),
        .cache = (base_path / toString(SectorFileType::FTCache)
                  / primitives::sector_file::sectorName(sector_.id))
                     .string(),
    };
    {
      std::ofstream o(paths.sealed);
      ASSERT_TRUE(o.good());
      o.close();
    }

    SectorPaths storages{
        .id = sector_.id,
        .unsealed = "",
        .sealed = "sealed-id",
        .cache = "cache-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    bool is_clear2 = false;
    EXPECT_CALL(
        *local_store_,
        reserve(sector_, SectorFileType::FTNone, storages, PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear2 = true; }));

    EXPECT_CALL(
        *store_,
        acquireSector(sector_,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(resp)));

    EXPECT_CALL(*store_, removeCopies(sector_.id, SectorFileType::FTSealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*store_, removeCopies(sector_.id, SectorFileType::FTCache))
        .WillOnce(testing::Return(outcome::success()));

    std::vector<char> some_bytes(127);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for (size_t i = 0; i < some_bytes.size(); i++) {
      some_bytes[i] = dis(gen);
    }

    auto offset{127};
    UnpaddedPieceSize piece_size(127);

    EXPECT_CALL(*proof_engine_,
                doUnsealRange(sector_.proof_type,
                              paths.cache,
                              _,
                              _,
                              sector_.id.sector,
                              sector_.id.miner,
                              randomness,
                              unsealed_cid,
                              primitives::piece::paddedIndex(offset),
                              uint64_t(piece_size)))
        .WillOnce(
            testing::Invoke([&](RegisteredSealProof proof_type,
                                const std::string &cache_dir_path,
                                int seal_fd,
                                int unseal_fd,
                                SectorNumber sector_num,
                                ActorId miner_id,
                                const Ticket &ticket,
                                const UnsealedCID &unsealed_cid,
                                uint64_t offset,
                                uint64_t length) -> outcome::result<void> {
              if (unseal_fd == -1) {
                return proofs::ProofsError::kUnknown;
              }
              write(unseal_fd, some_bytes.data(), some_bytes.size());
              return outcome::success();
            }));

    MOCK_API(return_interface_, ReturnUnsealPiece);

    EXPECT_OUTCOME_TRUE(
        call_id,
        local_worker_->unsealPiece(
            sector_, offset, piece_size, randomness, unsealed_cid));

    EXPECT_CALL(mock_ReturnUnsealPiece, Call(call_id, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();
    ASSERT_TRUE(is_clear);
    ASSERT_TRUE(is_clear2);

    EXPECT_OUTCOME_TRUE(file,
                        SectorFile::openFile(unseal_paths.unsealed, max_size));

    int p[2];
    ASSERT_EQ(pipe(p), 0);

    PieceData in(p[0]);

    EXPECT_OUTCOME_EQ(file->read(PieceData(p[1]),
                                 primitives::piece::paddedIndex(offset),
                                 piece_size.padded()),
                      true);

    std::vector<char> data(piece_size);
    ASSERT_NE(read(in.getFd(), data.data(), data.size()), -1);
    ASSERT_EQ(data, some_bytes);
  }

  /**
   * @given local worker
   * @when when try to unseal
   * @then file is created and unsealed
   */
  TEST_F(LocalWorkerTest, UnsealPiece) {
    PaddedPieceSize max_size(sector_size_);

    SealRandomness randomness{{1, 2, 3}};

    CID unsealed_cid = "010001020001"_cid;

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      ASSERT_TRUE(fs::create_directories(base_path / toString(type)));
    }

    SectorPaths unseal_paths{
        .id = sector_.id,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector_.id))
                        .string(),
        .sealed = "",
        .cache = "",
    };

    SectorPaths unseal_storages{
        .id = sector_.id,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(
            outcome::failure(stores::StoreError::kNotFoundSector)));

    EXPECT_CALL(*sector_index_,
                storageDeclareSector(unseal_storages.unsealed,
                                     sector_.id,
                                     SectorFileType::FTUnsealed,
                                     false))
        .WillOnce(testing::Return(outcome::success()));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        SectorFileType::FTUnsealed,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTNone,
                              SectorFileType::FTUnsealed,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    SectorPaths paths{
        .id = sector_.id,
        .unsealed = "",
        .sealed = (base_path / toString(SectorFileType::FTSealed)
                   / primitives::sector_file::sectorName(sector_.id))
                      .string(),
        .cache = (base_path / toString(SectorFileType::FTCache)
                  / primitives::sector_file::sectorName(sector_.id))
                     .string(),
    };
    {
      std::ofstream o(paths.sealed);
      ASSERT_TRUE(o.good());
      o.close();
    }

    SectorPaths storages{
        .id = sector_.id,
        .unsealed = "",
        .sealed = "sealed-id",
        .cache = "cache-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    bool is_clear2 = false;
    EXPECT_CALL(
        *local_store_,
        reserve(sector_, SectorFileType::FTNone, storages, PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear2 = true; }));

    EXPECT_CALL(
        *store_,
        acquireSector(sector_,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(resp)));

    EXPECT_CALL(*store_, removeCopies(sector_.id, SectorFileType::FTSealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*store_, removeCopies(sector_.id, SectorFileType::FTCache))
        .WillOnce(testing::Return(outcome::success()));

    std::vector<char> some_bytes(127);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for (size_t i = 0; i < some_bytes.size(); i++) {
      some_bytes[i] = dis(gen);
    }

    auto offset{127};
    UnpaddedPieceSize piece_size(127);

    EXPECT_CALL(*proof_engine_,
                doUnsealRange(sector_.proof_type,
                              paths.cache,
                              _,
                              _,
                              sector_.id.sector,
                              sector_.id.miner,
                              randomness,
                              unsealed_cid,
                              primitives::piece::paddedIndex(offset),
                              uint64_t(piece_size)))
        .WillOnce(
            testing::Invoke([&](RegisteredSealProof proof_type,
                                const std::string &cache_dir_path,
                                int seal_fd,
                                int unseal_fd,
                                SectorNumber sector_num,
                                ActorId miner_id,
                                const Ticket &ticket,
                                const UnsealedCID &unsealed_cid,
                                uint64_t offset,
                                uint64_t length) -> outcome::result<void> {
              if (unseal_fd == -1) {
                return proofs::ProofsError::kUnknown;
              }
              write(unseal_fd, some_bytes.data(), some_bytes.size());
              return outcome::success();
            }));

    MOCK_API(return_interface_, ReturnUnsealPiece);

    EXPECT_OUTCOME_TRUE(
        call_id,
        local_worker_->unsealPiece(
            sector_, offset, piece_size, randomness, unsealed_cid));

    EXPECT_CALL(mock_ReturnUnsealPiece, Call(call_id, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();
    ASSERT_TRUE(is_clear);
    ASSERT_TRUE(is_clear2);

    EXPECT_OUTCOME_TRUE(file,
                        SectorFile::openFile(unseal_paths.unsealed, max_size));

    int p[2];
    ASSERT_EQ(pipe(p), 0);

    PieceData in(p[0]);

    EXPECT_OUTCOME_EQ(file->read(PieceData(p[1]),
                                 primitives::piece::paddedIndex(offset),
                                 piece_size.padded()),
                      true);

    std::vector<char> data(piece_size);
    ASSERT_NE(read(in.getFd(), data.data(), data.size()), -1);
    ASSERT_EQ(data, some_bytes);
  }

  /**
   * @given local worker, storage
   * @when when try to move storage
   * @then storage is moved
   */
  TEST_F(LocalWorkerTest, MoveStorage) {
    SectorFileType types = static_cast<SectorFileType>(
        SectorFileType::FTCache | SectorFileType::FTSealed);

    EXPECT_CALL(*store_, moveStorage(sector_, types))
        .WillOnce(testing::Return(outcome::success()));

    MOCK_API(return_interface_, ReturnMoveStorage);

    EXPECT_OUTCOME_TRUE(call_id, local_worker_->moveStorage(sector_, types))

    EXPECT_CALL(mock_ReturnMoveStorage, Call(call_id, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();
  }

  /**
   * @given local worker, sector in another storage
   * @when when try to fetch
   * @then file is fetched
   */
  TEST_F(LocalWorkerTest, Fetch) {
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    auto type{SectorFileType::FTSealed};

    auto path{PathType::kSealing};

    auto acquire{AcquireMode::kMove};

    SectorPaths paths{
        .id = sector,
        .unsealed = "",
        .sealed =
            (base_path / primitives::sector_file::sectorName(sector)).string(),
        .cache = "",
    };

    SectorPaths storages{
        .id = sector,
        .unsealed = "",
        .sealed = "sealed-id",
        .cache = "",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    bool is_clear = false;
    EXPECT_CALL(
        *local_store_,
        reserve(sector_, SectorFileType::FTNone, storages, PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTSealed,
                              SectorFileType::FTNone,
                              path,
                              acquire))
        .WillOnce(testing::Return(outcome::success(resp)));

    MOCK_API(return_interface_, ReturnFetch);

    EXPECT_OUTCOME_TRUE(call_id,
                        local_worker_->fetch(sector_, type, path, acquire));

    EXPECT_CALL(mock_ReturnFetch, Call(call_id, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();

    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker
   * @when when try to read piece from not unsealed
   * @then piece is unsealed and read
   */
  TEST_F(LocalWorkerTest, readPieceNotExistFile) {
    auto offset{127};
    UnpaddedPieceSize piece_size(127);

    ASSERT_TRUE(fs::create_directories(base_path
                                       / toString(SectorFileType::FTUnsealed)));

    SectorPaths unseal_paths{
        .id = sector_.id,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector_.id))
                        .string(),
        .sealed = "",
        .cache = "",
    };

    SectorPaths unseal_storages{
        .id = sector_.id,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        SectorFileType::FTNone,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    std::string temp_path = (base_path / "temp").string();
    ASSERT_TRUE(std::ofstream(temp_path).good());

    MOCK_API(return_interface_, ReturnReadPiece);

    EXPECT_OUTCOME_TRUE(call_id,
                        local_worker_->readPiece(
                            PieceData(temp_path), sector_, offset, piece_size));

    bool status = true;
    EXPECT_CALL(mock_ReturnReadPiece, Call(call_id, _, Eq(boost::none)))
        .WillOnce(
            testing::Invoke([&](CallId call_id,
                                boost::optional<bool> maybe_status,
                                const boost::optional<CallError> &maybe_error)
                                -> outcome::result<void> {
              if (maybe_status.has_value()) {
                status = maybe_status.value();
                return outcome::success();
              }

              return ERROR_TEXT("ERROR: no value");
            }));

    io_context_->run_one();
    ASSERT_FALSE(status);

    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker, unsealed file(without piece)
   * @when when try to read not unsealed piece
   * @then piece is unsealed and read
   */
  TEST_F(LocalWorkerTest, readPieceNotAllocated) {
    PaddedPieceSize max_size(sector_size_);

    auto offset{127};
    UnpaddedPieceSize piece_size(127);

    ASSERT_TRUE(fs::create_directories(base_path
                                       / toString(SectorFileType::FTUnsealed)));

    SectorPaths unseal_paths{
        .id = sector_.id,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector_.id))
                        .string(),
        .sealed = "",
        .cache = "",
    };

    {
      EXPECT_OUTCOME_TRUE_1(
          SectorFile::createFile(unseal_paths.unsealed, max_size));
    }

    SectorPaths unseal_storages{
        .id = sector_.id,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        SectorFileType::FTNone,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    std::string temp_path = (base_path / "temp").string();
    ASSERT_TRUE(std::ofstream(temp_path).good());

    MOCK_API(return_interface_, ReturnReadPiece);

    EXPECT_OUTCOME_TRUE(call_id,
                        local_worker_->readPiece(
                            PieceData(temp_path), sector_, offset, piece_size));

    bool status = true;
    EXPECT_CALL(mock_ReturnReadPiece, Call(call_id, _, Eq(boost::none)))
        .WillOnce(
            testing::Invoke([&](const CallId &call_id,
                                boost::optional<bool> maybe_status,
                                const boost::optional<CallError> &maybe_error)
                                -> outcome::result<void> {
              if (maybe_status.has_value()) {
                status = maybe_status.value();
                return outcome::success();
              }

              return ERROR_TEXT("ERROR: no value");
            }));

    io_context_->run_one();
    ASSERT_FALSE(status);
    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker, unsealed file
   * @when when try to read  piece
   * @then piece is read
   */
  TEST_F(LocalWorkerTest, readPiece) {
    PaddedPieceSize max_size(sector_size_);

    auto offset{127};
    UnpaddedPieceSize piece_size(127);

    std::vector<char> some_bytes(piece_size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for (size_t i = 0; i < some_bytes.size(); i++) {
      some_bytes[i] = dis(gen);
    }

    ASSERT_TRUE(fs::create_directories(base_path
                                       / toString(SectorFileType::FTUnsealed)));

    SectorPaths unseal_paths{
        .id = sector_.id,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector_.id))
                        .string(),
        .sealed = "",
        .cache = "",
    };
    {
      EXPECT_OUTCOME_TRUE(
          file, SectorFile::createFile(unseal_paths.unsealed, max_size));

      std::string temp_path = (base_path / "temp").string();
      std::ofstream o(temp_path);
      ASSERT_TRUE(o.good());
      o.write(some_bytes.data(), some_bytes.size());
      o.close();
      EXPECT_OUTCOME_TRUE_1(file->write(PieceData(temp_path),
                                        UnpaddedPieceSize(offset).padded(),
                                        piece_size.padded()));

      EXPECT_OUTCOME_EQ(file->hasAllocated(offset, piece_size), true);
    }

    SectorPaths unseal_storages{
        .id = sector_.id,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        SectorFileType::FTNone,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    int p[2];
    ASSERT_EQ(pipe(p), 0);

    PieceData in(p[0]);

    MOCK_API(return_interface_, ReturnReadPiece);

    EXPECT_OUTCOME_TRUE(
        call_id,
        local_worker_->readPiece(PieceData(p[1]), sector_, offset, piece_size));

    bool status = false;
    EXPECT_CALL(mock_ReturnReadPiece, Call(call_id, _, Eq(boost::none)))
        .WillOnce(
            testing::Invoke([&](const CallId &call_id,
                                boost::optional<bool> maybe_status,
                                const boost::optional<CallError> &maybe_error)
                                -> outcome::result<void> {
              if (maybe_status.has_value()) {
                status = maybe_status.value();
                return outcome::success();
              }

              return ERROR_TEXT("ERROR: no error");
            }));

    io_context_->run_one();
    ASSERT_TRUE(status);
    ASSERT_TRUE(is_clear);

    std::vector<char> data(piece_size);
    ASSERT_NE(read(in.getFd(), data.data(), data.size()), -1);
    ASSERT_EQ(data, some_bytes);
  }

  /**
   * @given local worker, unsealed file, piece
   * @when when try to add piece more than max size
   * @then WorkerErrors::kOutOfBound occurs
   */
  TEST_F(LocalWorkerTest, AddPieceOutOfBound) {
    std::vector<UnpaddedPieceSize> pieces = {UnpaddedPieceSize(1016),
                                             UnpaddedPieceSize(1016)};

    MOCK_API(return_interface_, ReturnAddPiece);

    EXPECT_OUTCOME_TRUE(
        call_id,
        local_worker_->addPiece(
            sector_, pieces, UnpaddedPieceSize(127), PieceData("/dev/null")));

    CallError error;
    EXPECT_CALL(mock_ReturnAddPiece, Call(call_id, _, Ne(boost::none)))
        .WillOnce(
            testing::Invoke([&](const CallId &call_id,
                                boost::optional<PieceInfo> maybe_piece_info,
                                boost::optional<CallError> maybe_error)
                                -> outcome::result<void> {
              if (maybe_error.has_value()) {
                error = maybe_error.value();
                return outcome::success();
              }

              return ERROR_TEXT("ERROR: no value");
            }));

    io_context_->run_one();

    ASSERT_EQ(
        error.message,
        outcome::result<void>(WorkerErrors::kOutOfBound).error().message());
  }

  /**
   * @given local worker, unsealed file, piece
   * @when when try to add piece without exist pieces
   * @then file is created and piece is added
   */
  TEST_F(LocalWorkerTest, AddPieceWithoutExistPieces) {
    PaddedPieceSize max_size(sector_size_);

    auto result_cid =
        "baga6ea4seaqbart3og52jb2gmglvn5av45lm3i5gewpvf7clp5sb7jryk5prqcy";
    std::string data =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed "
        "do "
        "eiusmod tempor incididunt ut labore et dolore magna aliqua. "
        "Vi ";

    std::string input_path = (base_path / "temp").string();

    {
      std::ofstream out(input_path);
      ASSERT_TRUE(out.good());
      out.write(data.data(), data.size());
    }

    auto file_path =
        base_path / primitives::sector_file::sectorName(sector_.id);
    UnpaddedPieceSize piece_size(data.size());
    ASSERT_TRUE(piece_size.validate());

    SectorPaths unsealed_paths{
        .id = sector_.id,
        .unsealed = file_path.string(),
        .sealed = "",
        .cache = "",
    };

    SectorPaths unsealed_storages{
        .id = sector_.id,
        .unsealed = "unsealed-storage-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unsealed_resp{
        .paths = unsealed_paths,
        .storages = unsealed_storages,
    };

    EXPECT_CALL(*sector_index_,
                storageDeclareSector(unsealed_storages.unsealed,
                                     sector_.id,
                                     SectorFileType::FTUnsealed,
                                     false))
        .WillOnce(testing::Return(outcome::success()));

    bool is_clear_unsealed = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        SectorFileType::FTUnsealed,
                        unsealed_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear_unsealed = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTNone,
                              SectorFileType::FTUnsealed,

                              PathType::kSealing,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(unsealed_resp));

    MOCK_API(return_interface_, ReturnAddPiece);

    EXPECT_OUTCOME_TRUE(call_id,
                        local_worker_->addPiece(
                            sector_, {}, piece_size, PieceData(input_path)));

    PieceInfo info;
    EXPECT_CALL(mock_ReturnAddPiece, Call(call_id, _, Eq(boost::none)))
        .WillOnce(
            testing::Invoke([&](const CallId &call_id,
                                boost::optional<PieceInfo> maybe_piece_info,
                                const boost::optional<CallError> &maybe_error)
                                -> outcome::result<void> {
              if (maybe_piece_info.has_value()) {
                info = maybe_piece_info.value();
                return outcome::success();
              }

              return ERROR_TEXT("ERROR: no value");
            }));

    io_context_->run_one();

    ASSERT_TRUE(is_clear_unsealed);
    EXPECT_OUTCOME_EQ(info.cid.toString(), result_cid);
    ASSERT_EQ(info.size, piece_size.padded());
    EXPECT_OUTCOME_TRUE(file,
                        SectorFile::openFile(file_path.string(), max_size));

    int p[2];
    ASSERT_EQ(pipe(p), 0);

    PieceData in(p[0]);
    EXPECT_OUTCOME_EQ(file->read(PieceData(p[1]), 0, piece_size.padded()),
                      true);

    std::vector<char> read_data(piece_size);
    ASSERT_NE(read(in.getFd(), read_data.data(), piece_size), -1);

    ASSERT_EQ(gsl::make_span<char>(read_data.data(), read_data.size()),
              gsl::make_span<char>(data.data(), data.size()));
  }

  /**
   * @given local worker, unsealed file, piece, exist pieces
   * @when when try to add piece with exist pieces
   * @then file is opened and piece is added
   */
  TEST_F(LocalWorkerTest, AddPieceWithExistPieces) {
    PaddedPieceSize max_size(sector_size_);

    auto result_cid =
        "baga6ea4seaqbart3og52jb2gmglvn5av45lm3i5gewpvf7clp5sb7jryk5prqcy";
    std::string data =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed "
        "do "
        "eiusmod tempor incididunt ut labore et dolore magna aliqua. "
        "Vi ";

    std::string input_path = (base_path / "temp").string();

    {
      std::ofstream out(input_path);
      ASSERT_TRUE(out.good());
      out.write(data.data(), data.size());
    }

    auto file_path =
        base_path / primitives::sector_file::sectorName(sector_.id);

    {
      EXPECT_OUTCOME_TRUE_1(
          SectorFile::createFile(file_path.string(), max_size));
    }

    UnpaddedPieceSize piece_size(data.size());
    ASSERT_TRUE(piece_size.validate());

    SectorPaths unsealed_paths{
        .id = sector_.id,
        .unsealed = file_path.string(),
        .sealed = "",
        .cache = "",
    };

    SectorPaths unsealed_storages{
        .id = sector_.id,
        .unsealed = "unsealed-storage-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unsealed_resp{
        .paths = unsealed_paths,
        .storages = unsealed_storages,
    };

    bool is_clear_unsealed = false;
    EXPECT_CALL(*local_store_,
                reserve(sector_,
                        SectorFileType::FTNone,
                        unsealed_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear_unsealed = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kSealing,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(unsealed_resp));

    std::vector<UnpaddedPieceSize> pieces = {UnpaddedPieceSize(127)};

    MOCK_API(return_interface_, ReturnAddPiece);

    EXPECT_OUTCOME_TRUE(
        call_id,
        local_worker_->addPiece(
            sector_, pieces, piece_size, PieceData(input_path)));

    PieceInfo info;
    EXPECT_CALL(mock_ReturnAddPiece, Call(call_id, _, Eq(boost::none)))
        .WillOnce(
            testing::Invoke([&](const CallId &call_id,
                                boost::optional<PieceInfo> maybe_piece_info,
                                const boost::optional<CallError> &maybe_error)
                                -> outcome::result<void> {
              if (maybe_piece_info.has_value()) {
                info = maybe_piece_info.value();
                return outcome::success();
              }

              return ERROR_TEXT("ERROR: no value");
            }));

    io_context_->run_one();

    ASSERT_TRUE(is_clear_unsealed);
    EXPECT_OUTCOME_EQ(info.cid.toString(), result_cid);
    ASSERT_EQ(info.size, piece_size.padded());
    EXPECT_OUTCOME_TRUE(file,
                        SectorFile::openFile(file_path.string(), max_size));

    int p[2];
    ASSERT_EQ(pipe(p), 0);

    PieceData in(p[0]);
    EXPECT_OUTCOME_EQ(file->read(PieceData(p[1]),
                                 primitives::piece::paddedIndex(pieces[0]),
                                 piece_size.padded()),
                      true);

    std::vector<char> read_data(piece_size);
    ASSERT_NE(read(in.getFd(), read_data.data(), piece_size), -1);

    ASSERT_EQ(gsl::make_span<char>(read_data.data(), read_data.size()),
              gsl::make_span<char>(data.data(), data.size()));
  }

  TEST_F(LocalWorkerTest, ReplicaUpdate) {
    EXPECT_OUTCOME_TRUE(update_proof,
                        getRegisteredUpdateProof(sector_.proof_type));
    auto input_path = (base_path / "temp");

    auto sector_file = primitives::sector_file::sectorName(sector_.id);
    auto update_path =
        (input_path / toString(SectorFileType::FTUpdate) / sector_file)
            .string();
    auto update_cache_path =
        (input_path / toString(SectorFileType::FTUpdateCache) / sector_file)
            .string();
    auto unsealed_path =
        (input_path / toString(SectorFileType::FTUnsealed) / sector_file)
            .string();
    auto sealed_path =
        (input_path / toString(SectorFileType::FTSealed) / sector_file)
            .string();
    auto cache_path =
        (input_path / toString(SectorFileType::FTCache) / sector_file).string();

    SectorPaths paths{
        .id = sector_.id,
        .unsealed = unsealed_path,
        .sealed = sealed_path,
        .cache = cache_path,
        .update = update_path,
        .update_cache = update_cache_path,
    };

    boost::system::error_code ec;
    boost::filesystem::create_directories(
        input_path / toString(SectorFileType::FTSealed), ec);
    ASSERT_FALSE(ec.failed()) << ec.message();
    boost::filesystem::create_directories(
        input_path / toString(SectorFileType::FTUpdate), ec);
    ASSERT_FALSE(ec.failed()) << ec.message();
    boost::filesystem::create_directories(
        input_path / toString(SectorFileType::FTUpdateCache), ec);
    ASSERT_FALSE(ec.failed()) << ec.message();
    std::ofstream seal_file(sealed_path);
    ASSERT_TRUE(seal_file.good());
    seal_file.close();
    boost::filesystem::resize_file(sealed_path, sector_size_);

    auto storage = "some-uuid";

    SectorPaths storages{
        .id = sector_.id,
        .unsealed = storage,
        .sealed = storage,
        .cache = storage,
        .update = storage,
        .update_cache = storage,
    };

    AcquireSectorResponse response{
        .paths = paths,
        .storages = storages,
    };

    EXPECT_CALL(*sector_index_,
                storageDeclareSector(
                    storage, sector_.id, SectorFileType::FTUpdate, false))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*sector_index_,
                storageDeclareSector(
                    storage, sector_.id, SectorFileType::FTUpdateCache, false))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(
        *store_,
        acquireSector(sector_,
                      SectorFileType::FTUnsealed | SectorFileType::FTSealed
                          | SectorFileType::FTCache,
                      SectorFileType::FTUpdate | SectorFileType::FTUpdateCache,
                      PathType::kSealing,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(response));

    bool is_clear_called = false;
    EXPECT_CALL(
        *local_store_,
        reserve(sector_,
                SectorFileType::FTUpdate | SectorFileType::FTUpdateCache,
                storages,
                PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear_called = true; }));

    CID new_sealed_cid = "010001020001"_cid;
    CID new_unsealed_cid = "010001020002"_cid;

    SectorCids cids{.sealed_cid = new_sealed_cid,
                    .unsealed_cid = new_unsealed_cid};

    EXPECT_CALL(*proof_engine_,
                updateSeal(update_proof,
                           update_path,
                           update_cache_path,
                           sealed_path,
                           cache_path,
                           unsealed_path,
                           gsl::span<const PieceInfo>()))
        .WillOnce(testing::Return(outcome::success(cids)));

    MOCK_API(return_interface_, ReturnReplicaUpdate);

    EXPECT_OUTCOME_TRUE(call_id, local_worker_->replicaUpdate(sector_, {}));

    EXPECT_CALL(mock_ReturnReplicaUpdate, Call(call_id, cids, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();

    ASSERT_TRUE(is_clear_called);
    ASSERT_TRUE(boost::filesystem::exists(update_path));
    ASSERT_TRUE(boost::filesystem::exists(update_cache_path)
                and boost::filesystem::is_directory(update_cache_path));
  }

  TEST_F(LocalWorkerTest, proveReplicaUpdate1) {
    EXPECT_OUTCOME_TRUE(update_proof,
                        getRegisteredUpdateProof(sector_.proof_type));
    auto input_path = (base_path / "temp");

    auto sector_file = primitives::sector_file::sectorName(sector_.id);
    auto update_path =
        (input_path / toString(SectorFileType::FTUpdate) / sector_file)
            .string();
    auto update_cache_path =
        (input_path / toString(SectorFileType::FTUpdateCache) / sector_file)
            .string();
    auto sealed_path =
        (input_path / toString(SectorFileType::FTSealed) / sector_file)
            .string();
    auto cache_path =
        (input_path / toString(SectorFileType::FTCache) / sector_file).string();

    SectorPaths paths{
        .id = sector_.id,
        .unsealed = "",
        .sealed = sealed_path,
        .cache = cache_path,
        .update = update_path,
        .update_cache = update_cache_path,
    };

    auto storage = "some-uuid";

    SectorPaths storages{
        .id = sector_.id,
        .unsealed = "",
        .sealed = storage,
        .cache = storage,
        .update = storage,
        .update_cache = storage,
    };

    AcquireSectorResponse response{
        .paths = paths,
        .storages = storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector_,
                              SectorFileType::FTSealed | SectorFileType::FTCache
                                  | SectorFileType::FTUpdate
                                  | SectorFileType::FTUpdateCache,
                              SectorFileType::FTNone,
                              PathType::kSealing,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(response));

    bool is_clear_called = false;
    EXPECT_CALL(
        *local_store_,
        reserve(sector_, SectorFileType::FTNone, storages, PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear_called = true; }));

    CID new_sealed_cid = "010001020001"_cid;
    CID new_unsealed_cid = "010001020002"_cid;
    CID old_sealed_cid = "010001020003"_cid;

    Update1Output proofs = {{1, 2, 3}, {4, 5, 6}};

    EXPECT_CALL(*proof_engine_,
                updateProve1(update_proof,
                             old_sealed_cid,
                             new_sealed_cid,
                             new_unsealed_cid,
                             update_path,
                             update_cache_path,
                             sealed_path,
                             cache_path))
        .WillOnce(testing::Return(outcome::success(proofs)));

    MOCK_API(return_interface_, ReturnProveReplicaUpdate1);

    EXPECT_OUTCOME_TRUE(
        call_id,
        local_worker_->proveReplicaUpdate1(
            sector_, old_sealed_cid, new_sealed_cid, new_unsealed_cid));

    EXPECT_CALL(mock_ReturnProveReplicaUpdate1,
                Call(call_id, proofs, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();

    ASSERT_TRUE(is_clear_called);
  }

  TEST_F(LocalWorkerTest, proveReplicaUpdate2) {
    EXPECT_OUTCOME_TRUE(update_proof,
                        getRegisteredUpdateProof(sector_.proof_type));

    CID new_sealed_cid = "010001020001"_cid;
    CID new_unsealed_cid = "010001020002"_cid;
    CID old_sealed_cid = "010001020003"_cid;

    Update1Output proofs = {{1, 2, 3}, {4, 5, 6}};

    Proof proof{1, 2, 3, 4, 5, 6};

    EXPECT_CALL(*proof_engine_,
                updateProve2(update_proof,
                             old_sealed_cid,
                             new_sealed_cid,
                             new_unsealed_cid,
                             proofs))
        .WillOnce(testing::Return(outcome::success(proof)));

    MOCK_API(return_interface_, ReturnProveReplicaUpdate2);

    EXPECT_OUTCOME_TRUE(
        call_id,
        local_worker_->proveReplicaUpdate2(
            sector_, old_sealed_cid, new_sealed_cid, new_unsealed_cid, proofs));

    EXPECT_CALL(mock_ReturnProveReplicaUpdate2,
                Call(call_id, proof, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success()));

    io_context_->run_one();
  }

}  // namespace fc::sector_storage
