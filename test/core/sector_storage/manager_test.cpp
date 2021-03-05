/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/manager_impl.hpp"

#include <boost/filesystem.hpp>
#include <gsl/span>
#include <sector_storage/stores/store_error.hpp>

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
  using stores::AcquireSectorResponse;
  using stores::LocalStorageMock;
  using stores::LocalStoreMock;
  using stores::RemoteStoreMock;
  using stores::SectorIndexMock;
  using stores::SectorPaths;
  using ::testing::_;

  class ManagerTest : public test::BaseFS_Test {
   public:
    ManagerTest() : test::BaseFS_Test("fc_manager_test") {
      seal_proof_type_ = RegisteredSealProof::StackedDrg2KiBV1;

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

      EXPECT_CALL(*scheduler_, getSealProofType())
          .WillRepeatedly(::testing::Return(seal_proof_type_));

      EXPECT_CALL(*scheduler_, doNewWorker(_))
          .WillRepeatedly(::testing::Return());

      worker_ = std::make_shared<WorkerMock>();

      proof_engine_ = std::make_shared<proofs::ProofEngineMock>();

      EXPECT_CALL(*proof_engine_, getGPUDevices())
          .WillRepeatedly(
              testing::Return(outcome::success(std::vector<std::string>())));

      auto maybe_manager =
          ManagerImpl::newManager(remote_store_,
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
    RegisteredSealProof seal_proof_type_;

    std::shared_ptr<proofs::ProofEngineMock> proof_engine_;
    std::shared_ptr<WorkerMock> worker_;
    std::shared_ptr<SectorIndexMock> sector_index_;
    std::shared_ptr<LocalStorageMock> local_storage_;
    std::shared_ptr<LocalStoreMock> local_store_;
    std::shared_ptr<RemoteStoreMock> remote_store_;
    std::shared_ptr<SchedulerMock> scheduler_;
    std::shared_ptr<Manager> manager_;
  };

  TEST_F(ManagerTest, SealPreCommit1) {
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    SealRandomness randomness({1, 2, 3});
    std::vector<PieceInfo> pieces = {
        PieceInfo{
            .cid = "010001020001"_cid,
            .size = PaddedPieceSize(128),
        },
    };

    EXPECT_CALL(
        *sector_index_,
        storageLock(sector,
                    SectorFileType::FTUnsealed,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache)))
        .WillOnce(testing::Return(testing::ByMove(
            outcome::success(std::make_unique<stores::WLock>()))));

    std::vector<uint8_t> result = {1, 2, 3, 4, 5};

    EXPECT_CALL(*worker_,
                sealPreCommit1(sector,
                               randomness,
                               gsl::make_span<const PieceInfo>(pieces.data(),
                                                               pieces.size())))
        .WillOnce(testing::Return(outcome::success(result)));

    EXPECT_CALL(
        *scheduler_,
        schedule(
            sector, primitives::kTTPreCommit1, _, _, _, kDefaultTaskPriority))
        .WillOnce(
            testing::Invoke([&](const SectorId &sector,
                                const TaskType &task_type,
                                const std::shared_ptr<WorkerSelector> &selector,
                                const WorkerAction &prepare,
                                const WorkerAction &work,
                                uint64_t priority) -> outcome::result<void> {
              OUTCOME_TRY(work(worker_));
              return outcome::success();
            }));

    EXPECT_OUTCOME_EQ(manager_->sealPreCommit1(
                          sector, randomness, pieces, kDefaultTaskPriority),
                      result);
  }

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

  TEST_F(ManagerTest, CheckProvable) {
    EXPECT_OUTCOME_TRUE(ssize,
                        primitives::sector::getSectorSize(seal_proof_type_));
    auto sealed_path = base_path / toString(SectorFileType::FTSealed);
    auto cache_path = base_path / toString(SectorFileType::FTCache);

    ASSERT_TRUE(fs::create_directories(sealed_path));
    ASSERT_TRUE(fs::create_directories(cache_path));

    SectorId cannot_lock_sector{
        .miner = 42,
        .sector = 1,
    };

    EXPECT_CALL(
        *sector_index_,
        storageTryLock(cannot_lock_sector,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(testing::ByMove(nullptr)));

    SectorId non_exist_sector{
        .miner = 42,
        .sector = 2,
    };
    EXPECT_CALL(
        *sector_index_,
        storageTryLock(non_exist_sector,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    EXPECT_CALL(
        *local_store_,
        acquireSector(non_exist_sector,
                      seal_proof_type_,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(
            outcome::failure(stores::StoreErrors::kNotFoundSector)));

    SectorId non_exist_path_sector{
        .miner = 42,
        .sector = 3,
    };
    EXPECT_CALL(
        *sector_index_,
        storageTryLock(non_exist_path_sector,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    SectorPaths non_exist_path_paths{
        .id = non_exist_path_sector,
        .unsealed = "",
        .sealed = (sealed_path
                   / primitives::sector_file::sectorName(non_exist_path_sector))
                      .string(),
        .cache = (cache_path
                  / primitives::sector_file::sectorName(non_exist_path_sector))
                     .string(),
    };

    AcquireSectorResponse non_exist_path_response{
        .paths = non_exist_path_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *local_store_,
        acquireSector(non_exist_path_sector,
                      seal_proof_type_,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(outcome::success(non_exist_path_response)));

    SectorId wrong_size_sector{
        .miner = 42,
        .sector = 4,
    };
    EXPECT_CALL(
        *sector_index_,
        storageTryLock(wrong_size_sector,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    SectorPaths wrong_size_paths{
        .id = wrong_size_sector,
        .unsealed = "",
        .sealed = (sealed_path
                   / primitives::sector_file::sectorName(wrong_size_sector))
                      .string(),
        .cache = (cache_path
                  / primitives::sector_file::sectorName(wrong_size_sector))
                     .string(),
    };

    AcquireSectorResponse wrong_size_response{
        .paths = wrong_size_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *local_store_,
        acquireSector(wrong_size_sector,
                      seal_proof_type_,
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

    SectorId success_sector{
        .miner = 42,
        .sector = 5,
    };
    SectorPaths success_paths{
        .id = success_sector,
        .unsealed = "",
        .sealed =
            (sealed_path / primitives::sector_file::sectorName(success_sector))
                .string(),
        .cache =
            (cache_path / primitives::sector_file::sectorName(success_sector))
                .string(),
    };

    AcquireSectorResponse success_response{
        .paths = success_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *sector_index_,
        storageTryLock(success_sector,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    EXPECT_CALL(
        *local_store_,
        acquireSector(success_sector,
                      seal_proof_type_,
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

    std::vector<SectorId> sectors = {success_sector,
                                     wrong_size_sector,
                                     non_exist_path_sector,
                                     non_exist_sector,
                                     cannot_lock_sector};

    EXPECT_OUTCOME_TRUE(bad_sectors,
                        manager_->checkProvable(seal_proof_type_, sectors));
    EXPECT_THAT(bad_sectors,
                testing::UnorderedElementsAre(wrong_size_sector,
                                              non_exist_path_sector,
                                              non_exist_sector,
                                              cannot_lock_sector));
  }

  TEST_F(ManagerTest, GetSectorSize) {
    EXPECT_OUTCOME_TRUE(ssize,
                        primitives::sector::getSectorSize(seal_proof_type_));
    ASSERT_EQ(manager_->getSectorSize(), ssize);
  }

  TEST_F(ManagerTest, GetSectorSizeWrongProofType) {
    auto proof = static_cast<RegisteredSealProof>(-1);

    EXPECT_CALL(*scheduler_, getSealProofType())
        .WillOnce(testing::Return(proof));

    EXPECT_OUTCOME_TRUE(manager,
                        ManagerImpl::newManager(remote_store_,
                                                scheduler_,
                                                SealerConfig{
                                                    .allow_precommit_1 = true,
                                                    .allow_precommit_2 = true,
                                                    .allow_commit = true,
                                                    .allow_unseal = true,
                                                }));

    ASSERT_EQ(manager->getSectorSize(), 0);
  }

  TEST_F(ManagerTest, GenerateWinningPoSt) {
    EXPECT_OUTCOME_TRUE(
        post_proof,
        primitives::sector::getRegisteredWinningPoStProof(seal_proof_type_));

    EXPECT_OUTCOME_TRUE(randomness,
                        PoStRandomness::fromString(std::string(32, '\xff')));

    auto new_randomness = randomness;
    new_randomness[31] &= 0x3f;

    std::vector<proofs::PoStProof> result = {{
        .registered_proof = post_proof,
        .proof = {1, 2, 3, 4, 5},
    }};

    uint64_t miner_id = 42;

    std::vector<SectorInfo> public_sectors = {
        SectorInfo{.registered_proof = seal_proof_type_,
                   .sector = 1,
                   .sealed_cid = "010001020001"_cid},
    };

    SectorId success_sector{
        .miner = miner_id,
        .sector = public_sectors[0].sector,
    };

    SectorPaths success_paths{
        .id = success_sector,
        .unsealed = "",
        .sealed = (base_path / toString(SectorFileType::FTSealed)
                   / primitives::sector_file::sectorName(success_sector))
                      .string(),
        .cache = (base_path / toString(SectorFileType::FTCache)
                  / primitives::sector_file::sectorName(success_sector))
                     .string(),
    };

    AcquireSectorResponse success_response{
        .paths = success_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *local_store_,
        acquireSector(success_sector,
                      seal_proof_type_,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(outcome::success(success_response)));

    EXPECT_CALL(
        *sector_index_,
        storageTryLock(success_sector,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    std::vector<proofs::PrivateSectorInfo> private_sector = {
        proofs::PrivateSectorInfo{.info = public_sectors[0],
                                  .cache_dir_path = success_paths.cache,
                                  .post_proof_type = post_proof,
                                  .sealed_sector_path = success_paths.sealed}};

    auto s = proofs::newSortedPrivateSectorInfo(private_sector);

    EXPECT_CALL(*proof_engine_,
                generateWinningPoSt(miner_id, s, new_randomness))
        .WillOnce(testing::Return(outcome::success(result)));

    EXPECT_OUTCOME_EQ(
        manager_->generateWinningPoSt(miner_id, public_sectors, randomness),
        result)
  }

  TEST_F(ManagerTest, GenerateWinningPoStWithSkip) {
    EXPECT_OUTCOME_TRUE(
        post_proof,
        primitives::sector::getRegisteredWinningPoStProof(seal_proof_type_));

    EXPECT_OUTCOME_TRUE(randomness,
                        PoStRandomness::fromString(std::string(32, '\xff')));

    auto new_randomness = randomness;
    new_randomness[31] &= 0x3f;

    std::vector<proofs::PoStProof> result = {{
        .registered_proof = post_proof,
        .proof = {1, 2, 3, 4, 5},
    }};

    uint64_t miner_id = 42;

    std::vector<SectorInfo> public_sectors = {
        SectorInfo{.registered_proof = seal_proof_type_,
                   .sector = 1,
                   .sealed_cid = "010001020001"_cid},
        SectorInfo{.registered_proof = seal_proof_type_,
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

    SectorId success_sector{
        .miner = miner_id,
        .sector = public_sectors[0].sector,
    };

    SectorPaths success_paths{
        .id = success_sector,
        .unsealed = "",
        .sealed = (base_path / toString(SectorFileType::FTSealed)
                   / primitives::sector_file::sectorName(success_sector))
                      .string(),
        .cache = (base_path / toString(SectorFileType::FTCache)
                  / primitives::sector_file::sectorName(success_sector))
                     .string(),
    };

    AcquireSectorResponse success_response{
        .paths = success_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *local_store_,
        acquireSector(success_sector,
                      seal_proof_type_,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(outcome::success(success_response)));

    EXPECT_CALL(
        *sector_index_,
        storageTryLock(success_sector,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    EXPECT_OUTCOME_ERROR(
        ManagerErrors::kSomeSectorSkipped,
        manager_->generateWinningPoSt(miner_id, public_sectors, randomness))
  }

  TEST_F(ManagerTest, GenerateWindowPoSt) {
    EXPECT_OUTCOME_TRUE(
        post_proof,
        primitives::sector::getRegisteredWindowPoStProof(seal_proof_type_));

    EXPECT_OUTCOME_TRUE(randomness,
                        PoStRandomness::fromString(std::string(32, '\xff')));

    auto new_randomness = randomness;
    new_randomness[31] &= 0x3f;

    std::vector<proofs::PoStProof> proof = {{
        .registered_proof = post_proof,
        .proof = {1, 2, 3, 4, 5},
    }};

    uint64_t miner_id = 42;

    std::vector<SectorInfo> public_sectors = {
        SectorInfo{.registered_proof = seal_proof_type_,
                   .sector = 1,
                   .sealed_cid = "010001020001"_cid},
        SectorInfo{.registered_proof = seal_proof_type_,
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

    SectorId success_sector{
        .miner = miner_id,
        .sector = public_sectors[0].sector,
    };

    SectorPaths success_paths{
        .id = success_sector,
        .unsealed = "",
        .sealed = (base_path / toString(SectorFileType::FTSealed)
                   / primitives::sector_file::sectorName(success_sector))
                      .string(),
        .cache = (base_path / toString(SectorFileType::FTCache)
                  / primitives::sector_file::sectorName(success_sector))
                     .string(),
    };

    AcquireSectorResponse success_response{
        .paths = success_paths,
        .storages = {},
    };

    EXPECT_CALL(
        *local_store_,
        acquireSector(success_sector,
                      seal_proof_type_,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache),
                      SectorFileType::FTNone,
                      PathType::kStorage,
                      AcquireMode::kMove))
        .WillOnce(testing::Return(outcome::success(success_response)));

    EXPECT_CALL(
        *sector_index_,
        storageTryLock(success_sector,
                       static_cast<SectorFileType>(SectorFileType::FTSealed
                                                   | SectorFileType::FTCache),
                       SectorFileType::FTNone))
        .WillOnce(testing::Return(
            testing::ByMove(std::make_unique<stores::WLock>())));

    std::vector<proofs::PrivateSectorInfo> private_sector = {
        proofs::PrivateSectorInfo{.info = public_sectors[0],
                                  .cache_dir_path = success_paths.cache,
                                  .post_proof_type = post_proof,
                                  .sealed_sector_path = success_paths.sealed}};

    auto s = proofs::newSortedPrivateSectorInfo(private_sector);

    EXPECT_CALL(*proof_engine_, generateWindowPoSt(miner_id, s, new_randomness))
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
