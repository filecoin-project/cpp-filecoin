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

#include "proofs/proofs_error.hpp"
#include "sector_storage/stores/store_error.hpp"
#include "testutil/literals.hpp"
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
  using primitives::StoragePath;
  using primitives::piece::PaddedPieceSize;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::InteractiveRandomness;
  using primitives::sector::RegisteredSealProof;
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

  class LocalWorkerTest : public test::BaseFS_Test {
   public:
    LocalWorkerTest() : test::BaseFS_Test("fc_local_worker_test") {
      tasks_ = {
          primitives::kTTAddPiece,
          primitives::kTTPreCommit1,
          primitives::kTTPreCommit2,
      };
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;

      config_ = WorkerConfig{.seal_proof_type = seal_proof_type_,
                             .task_types = tasks_};
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

      local_worker_ =
          std::make_unique<LocalWorker>(config_, store_, proof_engine_);
    }

   protected:
    std::set<primitives::TaskType> tasks_;
    RegisteredSealProof seal_proof_type_;
    WorkerConfig config_;
    std::shared_ptr<RemoteStoreMock> store_;
    std::shared_ptr<LocalStoreMock> local_store_;
    std::shared_ptr<SectorIndexMock> sector_index_;
    std::shared_ptr<ProofEngineMock> proof_engine_;
    std::unique_ptr<LocalWorker> local_worker_;
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
    ASSERT_EQ(info.hostname, boost::asio::ip::host_name());
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
    SectorId sector{
        .miner = 1,
        .sector = 3,
    };

    SealRandomness ticket{{5, 4, 2}};

    AcquireSectorResponse response{};

    response.paths.sealed = (base_path / toString(SectorFileType::FTSealed)
                             / primitives::sector_file::sectorName(sector))
                                .string();
    response.paths.cache = (base_path / toString(SectorFileType::FTCache)
                            / primitives::sector_file::sectorName(sector))
                               .string();

    EXPECT_CALL(*store_, remove(sector, SectorFileType::FTSealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*store_, remove(sector, SectorFileType::FTCache))
        .WillOnce(testing::Return(outcome::success()));

    bool is_storage_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        static_cast<SectorFileType>(SectorFileType::FTCache
                                                    | SectorFileType::FTSealed),
                        _,
                        PathType::kSealing))
        .WillOnce(testing::Return(
            outcome::success([&]() { is_storage_clear = true; })));

    EXPECT_CALL(
        *sector_index_,
        storageDeclareSector(
            response.storages.cache, sector, SectorFileType::FTCache, false))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(
        *sector_index_,
        storageDeclareSector(
            response.storages.sealed, sector, SectorFileType::FTSealed, false))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(
        *store_,
        acquireSector(sector,
                      config_.seal_proof_type,
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

    EXPECT_OUTCOME_ERROR(WorkerErrors::kPiecesDoNotMatchSectorSize,
                         local_worker_->sealPreCommit1(sector, ticket, pieces));

    ASSERT_TRUE(is_storage_clear);
  }

  /**
   * @given local worker
   * @when when try to finalize sector without keep unsealed
   * @then sector file is removed
   */
  TEST_F(LocalWorkerTest, FinalizeSectorWithoutKeepUnsealed) {
    EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    SectorPaths paths{
        .id = sector,
        .unsealed = "",
        .sealed = "",
        .cache = "some/cache/path",
    };

    SectorPaths storages{
        .id = sector,
        .unsealed = "",
        .sealed = "",
        .cache = "cache-storage-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTCache,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(resp));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(*proof_engine_, clearCache(size, paths.cache))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*store_, remove(sector, SectorFileType::FTUnsealed))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_worker_->finalizeSector(sector, {}));
    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker
   * @when when try to finalize sector with keep unsealed
   * @then sector file is keeped
   */
  TEST_F(LocalWorkerTest, FinalizeSectorWithKeepUnsealed) {
    EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));

    std::vector<Range> ranges = {
        Range{
            .offset = UnpaddedPieceSize(0),
            .size = UnpaddedPieceSize(127),
        },
    };

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    auto file_path = base_path / primitives::sector_file::sectorName(sector);
    PaddedPieceSize piece_size(256);
    PaddedPieceSize max_size(size);
    {
      EXPECT_OUTCOME_TRUE(file,
                          SectorFile::createFile(file_path.string(), max_size));

      EXPECT_OUTCOME_TRUE_1(file->write(
          PieceData("/dev/random"), PaddedPieceSize(0), piece_size));

      EXPECT_OUTCOME_EQ(file->hasAllocated(0, piece_size.unpadded()), true);
    }

    SectorPaths unsealed_paths{
        .id = sector,
        .unsealed = file_path.string(),
        .sealed = "",
        .cache = "",
    };

    SectorPaths unsealed_storages{
        .id = sector,
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
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        unsealed_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear_unsealed = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(unsealed_resp));

    SectorPaths cache_paths{
        .id = sector,
        .unsealed = "",
        .sealed = "",
        .cache = "some/cache/path",
    };

    SectorPaths cache_storages{
        .id = sector,
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
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        cache_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTCache,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(cache_resp));

    EXPECT_CALL(*proof_engine_, clearCache(size, cache_paths.cache))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_worker_->finalizeSector(sector, ranges));
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
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    proofs::Ticket randomness{{1, 2, 3}};

    std::vector<PieceInfo> pieces = {PieceInfo{
                                         .size = PaddedPieceSize(1024),
                                         .cid = CID(),
                                     },
                                     PieceInfo{
                                         .size = PaddedPieceSize(1024),
                                         .cid = CID(),
                                     }};

    EXPECT_CALL(*store_, remove(sector, SectorFileType::FTSealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*store_, remove(sector, SectorFileType::FTCache))
        .WillOnce(testing::Return(outcome::success()));

    bool is_storage_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        static_cast<SectorFileType>(SectorFileType::FTCache
                                                    | SectorFileType::FTSealed),
                        _,
                        PathType::kSealing))
        .WillOnce(testing::Return(
            outcome::success([&]() { is_storage_clear = true; })));

    auto cache{base_path / toString(SectorFileType::FTCache)};

    ASSERT_TRUE(fs::create_directories(cache));

    SectorPaths paths{
        .id = sector,
        .unsealed = "",
        .sealed =
            (base_path / primitives::sector_file::sectorName(sector)).string(),
        .cache = (cache / primitives::sector_file::sectorName(sector)).string(),
    };

    SectorPaths storages{
        .id = sector,
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
        acquireSector(sector,
                      config_.seal_proof_type,
                      SectorFileType::FTUnsealed,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      PathType::kSealing,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(resp)));

    EXPECT_CALL(
        *sector_index_,
        storageDeclareSector(
            resp.storages.cache, sector, SectorFileType::FTCache, false))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(
        *sector_index_,
        storageDeclareSector(
            resp.storages.sealed, sector, SectorFileType::FTSealed, false))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*proof_engine_,
                sealPreCommitPhase1(seal_proof_type_,
                                    resp.paths.cache,
                                    resp.paths.unsealed,
                                    resp.paths.sealed,
                                    sector.sector,
                                    sector.miner,
                                    randomness,
                                    gsl::make_span<const PieceInfo>(
                                        pieces.data(), pieces.size())))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(
        local_worker_->sealPreCommit1(sector, randomness, pieces));
    ASSERT_TRUE(fs::exists(paths.sealed));
    ASSERT_TRUE(is_storage_clear);
  }

  /**
   * @given local worker, sector, precommit1 output
   * @when when try to seal pre commit 2
   * @then success
   */
  TEST_F(LocalWorkerTest, SealPreCommit2) {
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    auto cache{base_path / toString(SectorFileType::FTCache)};

    ASSERT_TRUE(fs::create_directories(cache));

    SectorPaths paths{
        .id = sector,
        .unsealed = "",
        .sealed =
            (base_path / primitives::sector_file::sectorName(sector)).string(),
        .cache = (cache / primitives::sector_file::sectorName(sector)).string(),
    };

    SectorPaths storages{
        .id = sector,
        .unsealed = "",
        .sealed = "sealed-id",
        .cache = "cache-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(
        *store_,
        acquireSector(sector,
                      config_.seal_proof_type,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kSealing,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(resp)));

    proofs::Phase1Output p1o = {0, 1, 2, 3};

    EXPECT_CALL(*proof_engine_,
                sealPreCommitPhase2(
                    gsl::make_span<const uint8_t>(p1o.data(), p1o.size()),
                    paths.cache,
                    paths.sealed))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_worker_->sealPreCommit2(sector, p1o));
    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker, sector, precommit2 output
   * @when when try to seal commit 1
   * @then success
   */
  TEST_F(LocalWorkerTest, SealCommit1) {
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };
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
        .id = sector,
        .unsealed = "",
        .sealed =
            (base_path / primitives::sector_file::sectorName(sector)).string(),
        .cache = (cache / primitives::sector_file::sectorName(sector)).string(),
    };

    SectorPaths storages{
        .id = sector,
        .unsealed = "",
        .sealed = "sealed-id",
        .cache = "cache-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(
        *store_,
        acquireSector(sector,
                      config_.seal_proof_type,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kSealing,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(resp)));

    EXPECT_CALL(*proof_engine_,
                sealCommitPhase1(seal_proof_type_,
                                 sealed_cid,
                                 unsealed_cid,
                                 paths.cache,
                                 paths.sealed,
                                 sector.sector,
                                 sector.miner,
                                 randomness,
                                 seed,
                                 gsl::make_span<const PieceInfo>(
                                     pieces.data(), pieces.size())))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(
        local_worker_->sealCommit1(sector, randomness, seed, pieces, cids));
    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker, sector, commit1 output
   * @when when try to seal commit 2
   * @then success
   */
  TEST_F(LocalWorkerTest, SealCommit2) {
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    Commit1Output c1o{{1, 2, 3}};

    EXPECT_CALL(
        *proof_engine_,
        sealCommitPhase2(gsl::make_span<const uint8_t>(c1o.data(), c1o.size()),
                         sector.sector,
                         sector.miner))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_worker_->sealCommit2(sector, c1o));
  }

  /**
   * @given local worker, unsealed sector file
   * @when when try to unseal already unsealed
   * @then success, without unseal twice
   */
  TEST_F(LocalWorkerTest, UnsealPieceAlreadyUnsealed) {
    EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));
    PaddedPieceSize max_size(size);

    auto offset{127};
    UnpaddedPieceSize piece_size(127);

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    SealRandomness randomness{{1, 2, 3}};

    CID unsealed_cid = "010001020001"_cid;

    ASSERT_TRUE(fs::create_directories(base_path
                                       / toString(SectorFileType::FTUnsealed)));

    SectorPaths unseal_paths{
        .id = sector,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector))
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
        .id = sector,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_OUTCOME_TRUE_1(local_worker_->unsealPiece(
        sector, offset, piece_size, randomness, unsealed_cid));
    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker, unsealed sector file(empty)
   * @when when try to unseal
   * @then file is unsealed
   */
  TEST_F(LocalWorkerTest, UnsealPieceAlreadyExistFile) {
    EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));
    PaddedPieceSize max_size(size);

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    SealRandomness randomness{{1, 2, 3}};

    CID unsealed_cid = "010001020001"_cid;

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      ASSERT_TRUE(fs::create_directories(base_path / toString(type)));
    }

    SectorPaths unseal_paths{
        .id = sector,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector))
                        .string(),
        .sealed = "",
        .cache = "",
    };

    {
      EXPECT_OUTCOME_TRUE_1(
          SectorFile::createFile(unseal_paths.unsealed, max_size));
    }

    SectorPaths unseal_storages{
        .id = sector,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    SectorPaths paths{
        .id = sector,
        .unsealed = "",
        .sealed = (base_path / toString(SectorFileType::FTSealed)
                   / primitives::sector_file::sectorName(sector))
                      .string(),
        .cache = (base_path / toString(SectorFileType::FTCache)
                  / primitives::sector_file::sectorName(sector))
                     .string(),
    };
    {
      std::ofstream o(paths.sealed);
      ASSERT_TRUE(o.good());
      o.close();
    }

    SectorPaths storages{
        .id = sector,
        .unsealed = "",
        .sealed = "sealed-id",
        .cache = "cache-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    bool is_clear2 = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear2 = true; }));

    EXPECT_CALL(
        *store_,
        acquireSector(sector,
                      config_.seal_proof_type,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(resp)));

    EXPECT_CALL(*store_, removeCopies(sector, SectorFileType::FTSealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*store_, removeCopies(sector, SectorFileType::FTCache))
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
                doUnsealRange(seal_proof_type_,
                              paths.cache,
                              _,
                              _,
                              sector.sector,
                              sector.miner,
                              randomness,
                              unsealed_cid,
                              primitives::piece::paddedIndex(offset),
                              uint64_t(piece_size.padded())))
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

    EXPECT_OUTCOME_TRUE_1(local_worker_->unsealPiece(
        sector, offset, piece_size, randomness, unsealed_cid));
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
    EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));
    PaddedPieceSize max_size(size);

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    SealRandomness randomness{{1, 2, 3}};

    CID unsealed_cid = "010001020001"_cid;

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      ASSERT_TRUE(fs::create_directories(base_path / toString(type)));
    }

    SectorPaths unseal_paths{
        .id = sector,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector))
                        .string(),
        .sealed = "",
        .cache = "",
    };

    SectorPaths unseal_storages{
        .id = sector,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(
            outcome::failure(stores::StoreError::kNotFoundSector)));

    EXPECT_CALL(*sector_index_,
                storageDeclareSector(unseal_storages.unsealed,
                                     sector,
                                     SectorFileType::FTUnsealed,
                                     false))
        .WillOnce(testing::Return(outcome::success()));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTUnsealed,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTNone,
                              SectorFileType::FTUnsealed,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    SectorPaths paths{
        .id = sector,
        .unsealed = "",
        .sealed = (base_path / toString(SectorFileType::FTSealed)
                   / primitives::sector_file::sectorName(sector))
                      .string(),
        .cache = (base_path / toString(SectorFileType::FTCache)
                  / primitives::sector_file::sectorName(sector))
                     .string(),
    };
    {
      std::ofstream o(paths.sealed);
      ASSERT_TRUE(o.good());
      o.close();
    }

    SectorPaths storages{
        .id = sector,
        .unsealed = "",
        .sealed = "sealed-id",
        .cache = "cache-id",
    };

    AcquireSectorResponse resp{
        .paths = paths,
        .storages = storages,
    };

    bool is_clear2 = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear2 = true; }));

    EXPECT_CALL(
        *store_,
        acquireSector(sector,
                      config_.seal_proof_type,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(resp)));

    EXPECT_CALL(*store_, removeCopies(sector, SectorFileType::FTSealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*store_, removeCopies(sector, SectorFileType::FTCache))
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
                doUnsealRange(seal_proof_type_,
                              paths.cache,
                              _,
                              _,
                              sector.sector,
                              sector.miner,
                              randomness,
                              unsealed_cid,
                              primitives::piece::paddedIndex(offset),
                              uint64_t(piece_size.padded())))
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

    EXPECT_OUTCOME_TRUE_1(local_worker_->unsealPiece(
        sector, offset, piece_size, randomness, unsealed_cid));
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
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    EXPECT_CALL(
        *store_,
        moveStorage(sector,
                    seal_proof_type_,
                    static_cast<SectorFileType>(SectorFileType::FTCache
                                                | SectorFileType::FTSealed)))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_worker_->moveStorage(sector))
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
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTSealed,
                              SectorFileType::FTNone,
                              path,
                              acquire))
        .WillOnce(testing::Return(outcome::success(resp)));

    EXPECT_OUTCOME_TRUE_1(local_worker_->fetch(sector, type, path, acquire));
    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker
   * @when when try to read piece from not unsealed
   * @then piece is unsealed and read
   */
  TEST_F(LocalWorkerTest, readPieceNotExistFile) {
    EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));
    PaddedPieceSize max_size(size);

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };
    auto offset{127};
    UnpaddedPieceSize piece_size(127);

    ASSERT_TRUE(fs::create_directories(base_path
                                       / toString(SectorFileType::FTUnsealed)));

    SectorPaths unseal_paths{
        .id = sector,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector))
                        .string(),
        .sealed = "",
        .cache = "",
    };

    SectorPaths unseal_storages{
        .id = sector,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    std::string temp_path = (base_path / "temp").string();
    ASSERT_TRUE(std::ofstream(temp_path).good());

    EXPECT_OUTCOME_EQ(local_worker_->readPiece(
                          PieceData(temp_path), sector, offset, piece_size),
                      false);
    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker, unsealed file(without piece)
   * @when when try to read not unsealed piece
   * @then piece is unsealed and read
   */
  TEST_F(LocalWorkerTest, readPieceNotAllocated) {
    EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));
    PaddedPieceSize max_size(size);

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };
    auto offset{127};
    UnpaddedPieceSize piece_size(127);

    ASSERT_TRUE(fs::create_directories(base_path
                                       / toString(SectorFileType::FTUnsealed)));

    SectorPaths unseal_paths{
        .id = sector,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector))
                        .string(),
        .sealed = "",
        .cache = "",
    };

    {
      EXPECT_OUTCOME_TRUE_1(
          SectorFile::createFile(unseal_paths.unsealed, max_size));
    }

    SectorPaths unseal_storages{
        .id = sector,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    std::string temp_path = (base_path / "temp").string();
    ASSERT_TRUE(std::ofstream(temp_path).good());

    EXPECT_OUTCOME_EQ(local_worker_->readPiece(
                          PieceData(temp_path), sector, offset, piece_size),
                      false);
    ASSERT_TRUE(is_clear);
  }

  /**
   * @given local worker, unsealed file
   * @when when try to read  piece
   * @then piece is read
   */
  TEST_F(LocalWorkerTest, readPiece) {
    EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));
    PaddedPieceSize max_size(size);

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };
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
        .id = sector,
        .unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                     / primitives::sector_file::sectorName(sector))
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
        .id = sector,
        .unsealed = "unsealed-id",
        .sealed = "",
        .cache = "",
    };

    AcquireSectorResponse unseal_resp{
        .paths = unseal_paths,
        .storages = unseal_storages,
    };

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kStorage,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(outcome::success(unseal_resp)));

    bool is_clear = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        unseal_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear = true; }));

    int p[2];
    ASSERT_EQ(pipe(p), 0);

    PieceData in(p[0]);

    EXPECT_OUTCOME_EQ(
        local_worker_->readPiece(PieceData(p[1]), sector, offset, piece_size),
        true);
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
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    std::vector<UnpaddedPieceSize> pieces = {UnpaddedPieceSize(1016),
                                             UnpaddedPieceSize(1016)};

    EXPECT_OUTCOME_ERROR(
        WorkerErrors::kOutOfBound,
        local_worker_->addPiece(
            sector, pieces, UnpaddedPieceSize(127), PieceData("/dev/null")));
  }

  /**
   * @given local worker, unsealed file, piece
   * @when when try to add piece without exist pieces
   * @then file is created and piece is added
   */
  TEST_F(LocalWorkerTest, AddPieceWithoutExistPieces) {
    EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));
    PaddedPieceSize max_size(size);

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

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

    auto file_path = base_path / primitives::sector_file::sectorName(sector);
    UnpaddedPieceSize piece_size(data.size());
    ASSERT_TRUE(piece_size.validate());

    SectorPaths unsealed_paths{
        .id = sector,
        .unsealed = file_path.string(),
        .sealed = "",
        .cache = "",
    };

    SectorPaths unsealed_storages{
        .id = sector,
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
                                     sector,
                                     SectorFileType::FTUnsealed,
                                     false))
        .WillOnce(testing::Return(outcome::success()));

    bool is_clear_unsealed = false;
    EXPECT_CALL(*local_store_,
                reserve(seal_proof_type_,
                        SectorFileType::FTUnsealed,
                        unsealed_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear_unsealed = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTNone,
                              SectorFileType::FTUnsealed,

                              PathType::kSealing,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(unsealed_resp));

    EXPECT_OUTCOME_TRUE(
        info,
        local_worker_->addPiece(sector, {}, piece_size, PieceData(input_path)));
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
    EXPECT_OUTCOME_TRUE(size, getSectorSize(seal_proof_type_));
    PaddedPieceSize max_size(size);

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

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

    auto file_path = base_path / primitives::sector_file::sectorName(sector);

    {
      EXPECT_OUTCOME_TRUE_1(
          SectorFile::createFile(file_path.string(), max_size));
    }

    UnpaddedPieceSize piece_size(data.size());
    ASSERT_TRUE(piece_size.validate());

    SectorPaths unsealed_paths{
        .id = sector,
        .unsealed = file_path.string(),
        .sealed = "",
        .cache = "",
    };

    SectorPaths unsealed_storages{
        .id = sector,
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
                reserve(seal_proof_type_,
                        SectorFileType::FTNone,
                        unsealed_storages,
                        PathType::kSealing))
        .WillOnce(testing::Return([&]() { is_clear_unsealed = true; }));

    EXPECT_CALL(*store_,
                acquireSector(sector,
                              seal_proof_type_,
                              SectorFileType::FTUnsealed,
                              SectorFileType::FTNone,
                              PathType::kSealing,
                              AcquireMode::kCopy))
        .WillOnce(testing::Return(unsealed_resp));

    std::vector<UnpaddedPieceSize> pieces = {UnpaddedPieceSize(127)};

    EXPECT_OUTCOME_TRUE(info,
                        local_worker_->addPiece(
                            sector, pieces, piece_size, PieceData(input_path)));
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

  /**
   * @given sector
   * @when try to remove sector
   * @then Success
   */
  TEST_F(LocalWorkerTest, Remove_Success) {
    SectorId sector{
        .miner = 1,
        .sector = 3,
    };

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      EXPECT_CALL(*store_, remove(sector, type))
          .WillOnce(testing::Return(outcome::success()));
    }

    EXPECT_OUTCOME_TRUE_1(local_worker_->remove(sector));
  }

  /**
   * @given sector
   * @when try to remove sector and one type removed with error
   * @then error occurs
   */
  TEST_F(LocalWorkerTest, Remove_Error) {
    SectorId sector{
        .miner = 1,
        .sector = 3,
    };

    EXPECT_CALL(*store_, remove(sector, SectorFileType::FTUnsealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*store_, remove(sector, SectorFileType::FTSealed))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*store_, remove(sector, SectorFileType::FTCache))
        .WillOnce(testing::Return(
            outcome::failure(stores::StoreError::kNotFoundSector)));

    EXPECT_OUTCOME_ERROR(WorkerErrors::kCannotRemoveSector,
                         local_worker_->remove(sector));
  }
}  // namespace fc::sector_storage
