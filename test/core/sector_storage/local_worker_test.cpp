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
#include "sector_storage/stores/store_error.hpp"
#include "testutil/mocks/proofs/proof_engine_mock.hpp"
#include "testutil/mocks/sector_storage/stores/local_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/remote_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::sector_storage {
  using primitives::StoragePath;
  using primitives::piece::PaddedPieceSize;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::InteractiveRandomness;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector_file::SectorFileType;
  using proofs::PieceInfo;
  using proofs::ProofEngineMock;
  using proofs::SealVerifyInfo;
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
      seal_proof_type_ = RegisteredSealProof::StackedDrg2KiBV1;

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

    EXPECT_OUTCOME_ERROR(WorkerErrors::kPiecesDoNotMatchSectorSize,
                         local_worker_->sealPreCommit1(sector, ticket, {}));

    ASSERT_TRUE(is_storage_clear);
  }

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
                          primitives::sector_file::SectorFile::createFile(
                              file_path.string(), max_size));

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
                        primitives::sector_file::SectorFile::openFile(
                            file_path.string(), max_size));
    EXPECT_OUTCOME_EQ(file->hasAllocated(0, piece_size.unpadded()), false);
    EXPECT_OUTCOME_EQ(file->hasAllocated(0, ranges[0].size), true);
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
            outcome::failure(stores::StoreErrors::kNotFoundSector)));

    EXPECT_OUTCOME_ERROR(WorkerErrors::kCannotRemoveSector,
                         local_worker_->remove(sector));
  }
}  // namespace fc::sector_storage
