/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/store.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <thread>

#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "common/outcome.hpp"
#include "sector_storage/stores/impl/local_store.hpp"
#include "sector_storage/stores/index.hpp"
#include "sector_storage/stores/store_error.hpp"
#include "testutil/mocks/libp2p/scheduler_mock.hpp"
#include "testutil/mocks/sector_storage/stores/local_storage_mock.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::sector_storage::stores {
  using libp2p::basic::ManualSchedulerBackend;
  using libp2p::basic::Scheduler;
  using libp2p::basic::SchedulerImpl;
  using primitives::FsStat;
  using primitives::LocalStorageMeta;
  using primitives::StorageID;
  using primitives::StoragePath;
  using primitives::sector_file::SectorFileType;
  using testing::_;
  using testing::Eq;

  void createMetaFile(const std::string &storage_path,
                      const LocalStorageMeta &meta) {
    boost::filesystem::path file(storage_path);
    file /= kMetaFileName;
    OUTCOME_EXCEPT(text, codec::json::format(api::encode(meta)));
    OUTCOME_EXCEPT(common::writeFile(file, text));
  }

  class LocalStoreTest : public test::BaseFS_Test {
   public:
    LocalStoreTest() : test::BaseFS_Test("fc_local_store_test") {
      sector_ = SectorRef{.id =
                              SectorId{
                                  .miner = 42,
                                  .sector = 1,
                              },
                          .proof_type = RegisteredSealProof::kStackedDrg2KiBV1};
      sector_size_ = getSectorSize(sector_.proof_type).value();
      index_ = std::make_shared<SectorIndexMock>();
      storage_ = std::make_shared<LocalStorageMock>();
      urls_ = {"http://url1.com", "http://url2.com"};

      EXPECT_CALL(*storage_, getStorage())
          .WillOnce(testing::Return(outcome::success(
              StorageConfig{.storage_paths = std::vector<LocalPath>({})})));

      EXPECT_CALL(*storage_, setStorage(_))
          .WillRepeatedly(testing::Return(outcome::success()));

      scheduler_backend_ = std::make_shared<ManualSchedulerBackend>();
      scheduler_ = std::make_shared<SchedulerImpl>(scheduler_backend_,
                                                   Scheduler::Config{});

      auto maybe_local =
          LocalStoreImpl::newLocalStore(storage_, index_, urls_, scheduler_);
      local_store_ = std::move(maybe_local.value());
    }

    void createStorage(const std::string &path,
                       const LocalStorageMeta &meta,
                       const FsStat &stat) {
      if (!boost::filesystem::exists(path)) {
        boost::filesystem::create_directory(path);
      }

      createMetaFile(path, meta);

      StorageInfo storage_info{
          .id = meta.id,
          .urls = urls_,
          .weight = 0,
          .can_seal = true,
          .can_store = true,
      };

      EXPECT_CALL(*storage_, getStat(path))
          .WillOnce(testing::Return(outcome::success(stat)));

      EXPECT_CALL(*index_, storageAttach(storage_info, stat))
          .WillOnce(testing::Return(outcome::success()));

      EXPECT_OUTCOME_TRUE_1(local_store_->openPath(path));
    }

   protected:
    SectorRef sector_;
    SectorSize sector_size_;
    std::shared_ptr<LocalStore> local_store_;
    std::shared_ptr<SectorIndexMock> index_;
    std::shared_ptr<LocalStorageMock> storage_;
    std::vector<std::string> urls_;
    std::shared_ptr<ManualSchedulerBackend> scheduler_backend_;
    std::shared_ptr<Scheduler> scheduler_;
  };

  /**
   * @given sector and registered proof type and sector file types
   * @when try to acquire sector with intersation in exisiting and allocate
   * types
   * @then StoreErrors::FindAndAllocate error occurs
   */
  TEST_F(LocalStoreTest, AcquireSectorFindAndAllocate) {
    auto file_type_allocate = static_cast<SectorFileType>(
        SectorFileType::FTCache | SectorFileType::FTUnsealed);

    auto file_type_existing = static_cast<SectorFileType>(
        SectorFileType::FTCache | SectorFileType::FTSealed);

    EXPECT_OUTCOME_ERROR(StoreError::kFindAndAllocate,
                         local_store_->acquireSector(sector_,
                                                     file_type_existing,
                                                     file_type_allocate,
                                                     PathType::kStorage,
                                                     AcquireMode::kCopy))
  }

  /**
   * @given sector and registered proof type
   * @when try to allocate for not existing storage
   * @then StoreErrors::NotFoundPath error occurs
   */
  TEST_F(LocalStoreTest, AcquireSectorNotFoundPath) {
    std::vector res = {StorageInfo{
        .id = "not_found_id",
        .urls = {},
        .weight = 0,
        .can_seal = false,
        .can_store = false,
    }};

    EXPECT_CALL(*index_,
                storageBestAlloc(SectorFileType::FTCache, sector_size_, false))
        .WillOnce(testing::Return(outcome::success(res)));

    EXPECT_OUTCOME_ERROR(StoreError::kNotFoundPath,
                         local_store_->acquireSector(sector_,
                                                     SectorFileType::FTNone,
                                                     SectorFileType::FTCache,
                                                     PathType::kStorage,
                                                     AcquireMode::kCopy))
  }

  /**
   * @given storage, sector, registered proof type
   * @when try to acquire sector for allocate
   * @then get id of the storage and specific path for that storage
   */
  TEST_F(LocalStoreTest, AcquireSectorAllocateSuccess) {
    SectorFileType file_type = SectorFileType::FTCache;

    auto storage_path = boost::filesystem::unique_path(
                            fs::canonical(base_path).append("%%%%%-storage"))
                            .string();

    StorageID storage_id = "storage_id";

    primitives::LocalStorageMeta storage_meta{
        .id = storage_id,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    FsStat stat{
        .capacity = 100,
        .available = 100,
        .reserved = 0,
    };

    createStorage(storage_path, storage_meta, stat);

    StorageInfo storage_info{
        .id = storage_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    std::vector res = {storage_info};

    EXPECT_CALL(*index_, storageBestAlloc(file_type, sector_size_, false))
        .WillOnce(testing::Return(outcome::success(res)));

    EXPECT_OUTCOME_TRUE(sectors,
                        local_store_->acquireSector(sector_,
                                                    SectorFileType::FTNone,
                                                    file_type,
                                                    PathType::kStorage,
                                                    AcquireMode::kCopy));

    std::string res_path =
        (boost::filesystem::path(storage_path) / toString(file_type)
         / primitives::sector_file::sectorName(sector_.id))
            .string();

    EXPECT_OUTCOME_EQ(sectors.paths.getPathByType(file_type), res_path);
    EXPECT_OUTCOME_EQ(sectors.storages.getPathByType(file_type), storage_id);
  }

  /**
   * @given storage, sector, registered proof type
   * @when try to acquire sector for find existing sector
   * @then get id of the storage and specific path for that storage
   */
  TEST_F(LocalStoreTest, AcqireSectorExistSuccess) {
    SectorFileType file_type = SectorFileType::FTCache;

    auto storage_path = boost::filesystem::unique_path(
                            fs::canonical(base_path).append("%%%%%-storage"))
                            .string();

    StorageID storage_id = "storage_id";

    primitives::LocalStorageMeta storage_meta{
        .id = storage_id,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    FsStat stat{
        .capacity = 200,
        .available = 200,
        .reserved = 0,
    };

    createStorage(storage_path, storage_meta, stat);

    StorageInfo storage_info{
        .id = storage_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    std::vector res = {storage_info};

    EXPECT_CALL(*index_,
                storageFindSector(sector_.id, file_type, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success(res)));

    EXPECT_OUTCOME_TRUE(sectors,
                        local_store_->acquireSector(sector_,
                                                    file_type,
                                                    SectorFileType::FTNone,
                                                    PathType::kStorage,
                                                    AcquireMode::kCopy));

    std::string res_path =
        (boost::filesystem::path(storage_path) / toString(file_type)
         / primitives::sector_file::sectorName(sector_.id))
            .string();

    EXPECT_OUTCOME_EQ(sectors.paths.getPathByType(file_type), res_path);
    EXPECT_OUTCOME_EQ(sectors.storages.getPathByType(file_type), storage_id);
  }

  /**
   * @given nothing
   * @when try to get stat for some not existing storage
   * @then StoreErrors::NotFoundStorage error occurs
   */
  TEST_F(LocalStoreTest, getFSStatNotFound) {
    EXPECT_OUTCOME_ERROR(StoreError::kNotFoundStorage,
                         local_store_->getFsStat("not_found_id"));
  }

  /**
   * @given storage
   * @when try to get stat for the storage
   * @then get stat of the storage from index
   */
  TEST_F(LocalStoreTest, getFSStatSuccess) {
    auto storage_path = boost::filesystem::unique_path(
                            fs::canonical(base_path).append("%%%%%-storage"))
                            .string();

    StorageID storage_id = "storage_id";

    primitives::LocalStorageMeta storage_meta{
        .id = storage_id,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    FsStat res_stat{
        .capacity = 100,
        .available = 100,
        .reserved = 0,
    };

    createStorage(storage_path, storage_meta, res_stat);

    EXPECT_CALL(*storage_, getStat(storage_path))
        .WillOnce(testing::Return(outcome::success(res_stat)));

    StorageInfo storage_info{
        .id = storage_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    std::vector res = {storage_info};

    EXPECT_OUTCOME_EQ(local_store_->getFsStat(storage_id), res_stat);
  }

  /**
   * @given storage with sector
   * @when open this storage for store
   * @then storage and sector successfully added
   */
  TEST_F(LocalStoreTest, openPathExistingSector) {
    auto storage_path = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));

    SectorFileType file_type = SectorFileType::FTCache;

    boost::filesystem::create_directories((storage_path / toString(file_type)));

    StorageID storage_id = "storage_id";

    primitives::LocalStorageMeta storage_meta{
        .id = storage_id,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    createMetaFile(storage_path.string(), storage_meta);

    FsStat stat{
        .capacity = 200,
        .available = 200,
        .reserved = 0,
    };

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    std::string sector_file = (storage_path / toString(file_type)
                               / primitives::sector_file::sectorName(sector))
                                  .string();

    std::ofstream(sector_file).close();

    EXPECT_CALL(*storage_, getStat(storage_path.string()))
        .WillOnce(testing::Return(outcome::success(stat)));

    EXPECT_CALL(*index_,
                storageAttach(
                    StorageInfo{
                        .id = storage_id,
                        .urls = urls_,
                        .weight = 0,
                        .can_seal = true,
                        .can_store = true,
                    },
                    stat))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*index_,
                storageDeclareSector(storage_id, sector, file_type, true))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path.string()));
  }

  /**
   * @given storage with sector with invalid name
   * @when open this storage for store
   * @then StoreErrors::InvalidSectorName error occurs
   */
  TEST_F(LocalStoreTest, openPathInvalidSectorName) {
    auto storage_path = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));

    SectorFileType file_type = SectorFileType::FTCache;

    boost::filesystem::create_directories((storage_path / toString(file_type)));

    StorageID storage_id = "storage_id";

    primitives::LocalStorageMeta storage_meta{
        .id = storage_id,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    createMetaFile(storage_path.string(), storage_meta);

    FsStat stat{
        .capacity = 200,
        .available = 200,
        .reserved = 0,
    };

    std::string sector_file =
        (storage_path / toString(file_type) / "s-t0-42").string();

    std::ofstream(sector_file).close();

    EXPECT_CALL(*storage_, getStat(storage_path.string()))
        .WillOnce(testing::Return(outcome::success(stat)));

    EXPECT_CALL(*index_,
                storageAttach(
                    StorageInfo{
                        .id = storage_id,
                        .urls = urls_,
                        .weight = 0,
                        .can_seal = true,
                        .can_store = true,
                    },
                    stat))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_ERROR(StoreError::kInvalidSectorName,
                         local_store_->openPath(storage_path.string()));
  }

  /**
   * @given storage in the store and same storage
   * @when open the same storage for store
   * @then StoreErrors::DuplicateStorage error occurs
   */
  TEST_F(LocalStoreTest, openPathDuplicateStorage) {
    auto storage_path = boost::filesystem::unique_path(
                            fs::canonical(base_path).append("%%%%%-storage"))
                            .string();

    primitives::LocalStorageMeta storage_meta{
        .id = "storage_id",
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    FsStat stat{
        .capacity = 100,
        .available = 100,
        .reserved = 0,
    };

    createStorage(storage_path, storage_meta, stat);

    EXPECT_OUTCOME_ERROR(StoreError::kDuplicateStorage,
                         local_store_->openPath(storage_path));
  }

  /**
   * @given storage with invalid config file
   * @when open this storage for store
   * @then StoreErrors::InvalidStorageConfig error occurs
   */
  TEST_F(LocalStoreTest, openPathInvalidConfig) {
    auto storage_path = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));

    boost::filesystem::create_directory(storage_path);

    std::ofstream file((storage_path / kMetaFileName).string());

    ASSERT_TRUE(file.good());

    file << "some not JSON info" << std::endl;

    file.close();

    EXPECT_FALSE(local_store_->openPath(storage_path.string()));
  }

  /**
   * @given storage withou config file
   * @when open this storage for store
   * @then StoreErrors::InvalidStorageConfig error occurs
   */
  TEST_F(LocalStoreTest, openPathNoConfig) {
    auto storage_path = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));

    boost::filesystem::create_directory(storage_path);

    EXPECT_FALSE(local_store_->openPath(storage_path.string()));
  }

  /**
   * @given sector and complex sector file type
   * @when try to remove sector files with this type
   * @then StoreErrors::RemoveSeveralFileTypes error occurs
   */
  TEST_F(LocalStoreTest, removeSeveralSectorTypes) {
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    auto type = static_cast<SectorFileType>(SectorFileType::FTCache
                                            | SectorFileType::FTUnsealed);

    EXPECT_OUTCOME_ERROR(StoreError::kRemoveSeveralFileTypes,
                         local_store_->remove(sector, type));

    EXPECT_OUTCOME_ERROR(StoreError::kRemoveSeveralFileTypes,
                         local_store_->remove(sector, SectorFileType::FTNone));
  }

  /**
   * @given non existing sector info
   * @when try to remove this sector
   * @then success
   */
  TEST_F(LocalStoreTest, removeNotExistSector) {
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    SectorFileType type = SectorFileType::FTCache;

    EXPECT_CALL(*index_, storageFindSector(sector, type, Eq(boost::none)))
        .WillOnce(
            testing::Return(outcome::success(std::vector<StorageInfo>())));

    EXPECT_OUTCOME_TRUE_1(local_store_->remove(sector, type));
  }

  /**
   * @given storage with sector
   * @when try to remove the sector
   * @then sector successfully deleted
   */
  TEST_F(LocalStoreTest, removeSuccess) {
    auto storage_path = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));

    SectorFileType file_type = SectorFileType::FTCache;

    boost::filesystem::create_directories((storage_path / toString(file_type)));

    StorageID storage_id = "storage_id";

    primitives::LocalStorageMeta storage_meta{
        .id = storage_id,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    createMetaFile(storage_path.string(), storage_meta);

    FsStat stat{
        .capacity = 200,
        .available = 200,
        .reserved = 0,
    };

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    std::string sector_file = (storage_path / toString(file_type)
                               / primitives::sector_file::sectorName(sector))
                                  .string();

    std::ofstream(sector_file).close();

    EXPECT_CALL(*storage_, getStat(storage_path.string()))
        .WillOnce(testing::Return(outcome::success(stat)));

    StorageInfo info{
        .id = storage_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    EXPECT_CALL(*index_, storageAttach(info, stat))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*index_,
                storageDeclareSector(storage_id, sector, file_type, true))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path.string()));

    std::vector<StorageInfo> res = {info};

    EXPECT_CALL(*index_, storageFindSector(sector, file_type, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success(res)));

    EXPECT_CALL(*index_, storageDropSector(storage_id, sector, file_type))
        .WillOnce(testing::Return(outcome::success()));

    ASSERT_TRUE(boost::filesystem::exists(sector_file));
    EXPECT_OUTCOME_TRUE_1(local_store_->remove(sector, file_type));
    ASSERT_FALSE(boost::filesystem::exists(sector_file));
  }

  /**
   * @given 2 storages, 1 sector in the first
   * @when try to move from one sector to another
   * @then sector successfuly moved
   */
  TEST_F(LocalStoreTest, moveStorageSuccess) {
    auto storage_path = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));

    SectorFileType file_type = SectorFileType::FTCache;

    boost::filesystem::create_directories((storage_path / toString(file_type)));

    StorageID storage_id = "storage_id";

    primitives::LocalStorageMeta storage_meta{
        .id = storage_id,
        .weight = 0,
        .can_seal = true,
        .can_store = false,
    };

    createMetaFile(storage_path.string(), storage_meta);

    FsStat stat{
        .capacity = 200,
        .available = 200,
        .reserved = 0,
    };

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    std::string sector_file = (storage_path / toString(file_type)
                               / primitives::sector_file::sectorName(sector))
                                  .string();

    std::ofstream(sector_file).close();

    EXPECT_CALL(*storage_, getStat(storage_path.string()))
        .WillOnce(testing::Return(outcome::success(stat)));

    StorageInfo info{
        .id = storage_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = false,
    };

    EXPECT_CALL(*index_, storageAttach(info, stat))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*index_,
                storageDeclareSector(storage_id, sector, file_type, false))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path.string()));

    auto storage_path_2 = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));

    StorageID storage_id2 = "storage_id2";

    primitives::LocalStorageMeta storage_meta2{
        .id = storage_id2,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    FsStat stat2{
        .capacity = 200,
        .available = 200,
        .reserved = 0,
    };

    createStorage(storage_path_2.string(), storage_meta2, stat2);

    EXPECT_CALL(*index_, getStorageInfo(storage_id))
        .WillOnce(testing::Return(outcome::success(info)));

    StorageInfo info2{
        .id = storage_id2,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    EXPECT_CALL(*index_, getStorageInfo(storage_id2))
        .WillOnce(testing::Return(outcome::success(info2)));

    EXPECT_CALL(*index_, storageDropSector(storage_id, sector, file_type))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*index_,
                storageDeclareSector(storage_id2, sector, file_type, true))
        .WillOnce(testing::Return(outcome::success()));

    std::string moved_sector_file =
        (storage_path_2 / toString(file_type)
         / primitives::sector_file::sectorName(sector))
            .string();

    ASSERT_FALSE(boost::filesystem::exists(moved_sector_file));
    ASSERT_TRUE(boost::filesystem::exists(sector_file));

    EXPECT_CALL(*index_, storageFindSector(sector, file_type, Eq(boost::none)))
        .WillOnce(testing::Return(outcome::success(std::vector({info}))));

    EXPECT_CALL(*index_, storageBestAlloc(file_type, sector_size_, false))
        .WillOnce(testing::Return(outcome::success(std::vector({info2}))));

    EXPECT_OUTCOME_TRUE_1(local_store_->moveStorage(sector_, file_type));

    ASSERT_TRUE(boost::filesystem::exists(moved_sector_file));
    ASSERT_FALSE(boost::filesystem::exists(sector_file));
  }

  /**
   * @given one storage
   * @when try to get storage health during the predefined time period
   * @then storage successfully reported one time about its health status
   */
  TEST_F(LocalStoreTest, storageHealthSuccess) {
    auto storage_path = boost::filesystem::unique_path(
                            fs::canonical(base_path).append("%%%%%-storage"))
                            .string();

    StorageID storage_id = "storage_id";

    primitives::LocalStorageMeta storage_meta{
        .id = storage_id,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
    };

    FsStat stat{
        .capacity = 200,
        .available = 200,
        .reserved = 0,
    };

    createStorage(storage_path, storage_meta, stat);
    EXPECT_CALL(*index_, storageReportHealth(storage_id, _))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*storage_, getStat(storage_path))
        .WillOnce(testing::Return(outcome::success(stat)));
    scheduler_backend_->shiftToTimer();
  }

  /**
   * @given sector and complex sector file type
   * @when try to remove copies of sector files with this type
   * @then StoreErrors::RemoveSeveralFileTypes error occurs
   */
  TEST_F(LocalStoreTest, removeCopiesSeveralSectorTypes) {
    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    auto type = static_cast<SectorFileType>(SectorFileType::FTCache
                                            | SectorFileType::FTUnsealed);

    EXPECT_OUTCOME_ERROR(StoreError::kRemoveSeveralFileTypes,
                         local_store_->removeCopies(sector, type));

    EXPECT_OUTCOME_ERROR(
        StoreError::kRemoveSeveralFileTypes,
        local_store_->removeCopies(sector, SectorFileType::FTNone));
  }

  /**
   * @given sector, sector file type, and 1 non primary storage info
   * @when try to remove copies of sector files with this type
   * @then nothing happend, beacuse non-primary
   */
  TEST_F(LocalStoreTest, removeCopiesWithoutPirmaryStorage) {
    auto storage_path = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    auto type = SectorFileType::FTCache;

    std::string non_primary_id = "someid";

    boost::filesystem::create_directories((storage_path / toString(type)));

    primitives::LocalStorageMeta storage_meta{
        .id = non_primary_id,
        .weight = 0,
        .can_seal = true,
        .can_store = false,
    };

    createMetaFile(storage_path.string(), storage_meta);

    FsStat stat{
        .capacity = 200,
        .available = 200,
        .reserved = 0,
    };

    StorageInfo info{
        .id = non_primary_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = false,
        .is_primary = false,
    };

    std::string sector_file = (storage_path / toString(type)
                               / primitives::sector_file::sectorName(sector))
                                  .string();

    std::ofstream(sector_file).close();

    EXPECT_CALL(*storage_, getStat(storage_path.string()))
        .WillOnce(testing::Return(outcome::success(stat)));

    EXPECT_CALL(*index_, storageAttach(info, stat))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*index_,
                storageDeclareSector(non_primary_id, sector, type, false))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path.string()));

    std::vector<StorageInfo> sector_infos = {info};

    EXPECT_CALL(*index_, storageFindSector(sector, type, _))
        .WillOnce(testing::Return(outcome::success(sector_infos)));

    ASSERT_TRUE(boost::filesystem::exists(sector_file));
    EXPECT_OUTCOME_TRUE_1(local_store_->removeCopies(sector, type));
    ASSERT_TRUE(boost::filesystem::exists(sector_file));
  }

  /**
   * @given sector, sector file type, and 1 non primary and 1 primary storage
   * infos
   * @when try to remove copies of sector files with this type
   * @then sector in non primary is removed, but in primary still there
   */
  TEST_F(LocalStoreTest, removeCopiesWithPirmaryStorage) {
    auto storage_path = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));
    auto storage_path1 = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    auto type = SectorFileType::FTCache;

    std::string non_primary_id = "someid";
    std::string primary_id = "someid2";

    boost::filesystem::create_directories((storage_path / toString(type)));
    boost::filesystem::create_directories((storage_path1 / toString(type)));

    primitives::LocalStorageMeta storage_meta{
        .id = non_primary_id,
        .weight = 0,
        .can_seal = true,
        .can_store = false,
    };
    createMetaFile(storage_path.string(), storage_meta);
    storage_meta.can_store = true;
    storage_meta.id = primary_id;
    createMetaFile(storage_path1.string(), storage_meta);

    FsStat stat{
        .capacity = 200,
        .available = 200,
        .reserved = 0,
    };

    StorageInfo info{
        .id = non_primary_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = false,
        .is_primary = false,
    };
    StorageInfo info1{
        .id = primary_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
        .is_primary = false,
    };

    std::string sector_file = (storage_path / toString(type)
                               / primitives::sector_file::sectorName(sector))
                                  .string();

    std::ofstream(sector_file).close();
    std::string sector_file1 = (storage_path1 / toString(type)
                                / primitives::sector_file::sectorName(sector))
                                   .string();

    std::ofstream(sector_file1).close();

    EXPECT_CALL(*storage_, getStat(storage_path.string()))
        .WillOnce(testing::Return(outcome::success(stat)));
    EXPECT_CALL(*storage_, getStat(storage_path1.string()))
        .WillOnce(testing::Return(outcome::success(stat)));

    EXPECT_CALL(*index_, storageAttach(info, stat))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*index_, storageAttach(info1, stat))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*index_,
                storageDeclareSector(non_primary_id, sector, type, false))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*index_, storageDeclareSector(primary_id, sector, type, true))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path.string()));
    EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path1.string()));

    info1.is_primary = true;

    std::vector<StorageInfo> sector_infos = {info, info1};

    EXPECT_CALL(*index_, storageFindSector(sector, type, _))
        .WillOnce(testing::Return(outcome::success(sector_infos)));

    EXPECT_CALL(*index_, storageDropSector(non_primary_id, sector, type))
        .WillOnce(testing::Return(outcome::success()));

    ASSERT_TRUE(boost::filesystem::exists(sector_file1));
    ASSERT_TRUE(boost::filesystem::exists(sector_file));
    EXPECT_OUTCOME_TRUE_1(local_store_->removeCopies(sector, type));
    ASSERT_TRUE(boost::filesystem::exists(sector_file1));
    ASSERT_FALSE(boost::filesystem::exists(sector_file));
  }

  /**
   * @given store, 2 storages
   * @when try to get accessible paths
   * @then got 2 root paths of storages
   */
  TEST_F(LocalStoreTest, getAccessiblePaths) {
    auto storage_path = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));
    auto storage_path1 = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));
    boost::filesystem::create_directories(storage_path);
    boost::filesystem::create_directories(storage_path1);

    std::string non_primary_id = "someid";
    std::string primary_id = "someid2";

    primitives::LocalStorageMeta storage_meta{
        .id = non_primary_id,
        .weight = 0,
        .can_seal = true,
        .can_store = false,
    };
    createMetaFile(storage_path.string(), storage_meta);
    storage_meta.can_store = true;
    storage_meta.id = primary_id;
    createMetaFile(storage_path1.string(), storage_meta);

    FsStat stat{
        .capacity = 200,
        .available = 200,
        .reserved = 0,
    };

    StorageInfo info{
        .id = non_primary_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = false,
        .is_primary = false,
    };
    StorageInfo info1{
        .id = primary_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = true,
        .is_primary = false,
    };

    EXPECT_CALL(*storage_, getStat(storage_path.string()))
        .WillOnce(testing::Return(outcome::success(stat)));
    EXPECT_CALL(*storage_, getStat(storage_path1.string()))
        .WillOnce(testing::Return(outcome::success(stat)));

    EXPECT_CALL(*index_, storageAttach(info, stat))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*index_, storageAttach(info1, stat))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path.string()));
    EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path1.string()));

    StoragePath path{
        .id = info1.id,
        .weight = info1.weight,
        .local_path = storage_path1.string(),
        .can_seal = info1.can_seal,
        .can_store = info1.can_store,
    };
    StoragePath path1{
        .id = info.id,
        .weight = info.weight,
        .local_path = storage_path.string(),
        .can_seal = info.can_seal,
        .can_store = info.can_store,
    };

    EXPECT_CALL(*index_, getStorageInfo(non_primary_id))
        .WillOnce(testing::Return(outcome::success(info)));
    EXPECT_CALL(*index_, getStorageInfo(primary_id))
        .WillOnce(testing::Return(outcome::success(info1)));

    EXPECT_OUTCOME_TRUE(paths, local_store_->getAccessiblePaths())
    ASSERT_THAT(paths, testing::ElementsAre(path, path1));
  }

  /**
   * @given store, index
   * @when try to get index
   * @then index is received
   */
  TEST_F(LocalStoreTest, getSectorIndex) {
    ASSERT_EQ(index_, local_store_->getSectorIndex());
  }

  /**
   * @given store, local storage
   * @when try to get storage
   * @then storage is received
   */
  TEST_F(LocalStoreTest, getLocalStorage) {
    ASSERT_EQ(storage_, local_store_->getLocalStorage());
  }

  /**
   * @given store
   * @when try to reserve
   * @then space is received
   */
  TEST_F(LocalStoreTest, reserve) {
    auto storage_path = boost::filesystem::unique_path(
        fs::canonical(base_path).append("%%%%%-storage"));

    SectorId sector{
        .miner = 42,
        .sector = 1,
    };

    auto type = SectorFileType::FTSealed;

    std::string storage_id = "someid";

    boost::filesystem::create_directories((storage_path / toString(type)));

    primitives::LocalStorageMeta storage_meta{
        .id = storage_id,
        .weight = 0,
        .can_seal = true,
        .can_store = false,
    };

    createMetaFile(storage_path.string(), storage_meta);

    FsStat stat{
        .capacity = 4048,
        .available = 4048,
        .reserved = 0,
    };

    StorageInfo info{
        .id = storage_id,
        .urls = urls_,
        .weight = 0,
        .can_seal = true,
        .can_store = false,
        .is_primary = false,
    };

    EXPECT_CALL(*storage_, getStat(storage_path.string()))
        .WillRepeatedly(testing::Return(outcome::success(stat)));

    EXPECT_CALL(*index_, storageAttach(info, stat))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path.string()));

    PathType path_type = PathType::kStorage;
    SectorPaths spaths{
        .id = sector,
    };
    spaths.setPathByType(type, storage_id);

    FsStat before_reserve;
    FsStat after_reserve;
    FsStat after_release;

    EXPECT_CALL(*index_, storageReportHealth(storage_id, _))
        .WillOnce(testing::Invoke([&](StorageID id, HealthReport report) {
          EXPECT_EQ(id, storage_id);
          before_reserve = report.stat;
          return outcome::success();
        }))
        .WillOnce(testing::Invoke([&](StorageID id, HealthReport report) {
          EXPECT_EQ(id, storage_id);
          after_reserve = report.stat;
          return outcome::success();
        }))
        .WillOnce(testing::Invoke([&](StorageID id, HealthReport report) {
          EXPECT_EQ(id, storage_id);
          after_release = report.stat;
          return outcome::success();
        }));

    scheduler_backend_->shiftToTimer();
    EXPECT_OUTCOME_TRUE(
        release, local_store_->reserve(sector_, type, spaths, path_type));

    std::string sector_file = (storage_path / toString(type)
                               / primitives::sector_file::sectorName(sector))
                                  .string();

    std::ofstream(sector_file).close();
    auto temp_file_size = 100;

    EXPECT_CALL(*storage_, getDiskUsage(sector_file))
        .WillRepeatedly(testing::Return(outcome::success(temp_file_size)));

    scheduler_backend_->shiftToTimer();
    // some error occurred and file was removed
    release();
    scheduler_backend_->shiftToTimer();
    ASSERT_EQ(before_reserve, after_release);
    ASSERT_EQ(after_reserve.reserved,
              before_reserve.available - after_reserve.available);
  }

  /**
   * @given storage, index, urls, scheduler
   * @when try to create store, but storage doesn't have config
   * @then error kConfigFileNotExist occurs
   */
  TEST_F(LocalStoreTest, noExistConfig) {
    EXPECT_CALL(*storage_, getStorage())
        .WillOnce(testing::Return(outcome::success(boost::none)));
    EXPECT_OUTCOME_ERROR(
        StoreError::kConfigFileNotExist,
        LocalStoreImpl::newLocalStore(storage_, index_, urls_, scheduler_));
  }

}  // namespace fc::sector_storage::stores
