/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/store.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>

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

using fc::primitives::FsStat;
using fc::primitives::LocalStorageMeta;
using fc::primitives::StorageID;
using fc::primitives::sector_file::SectorFileType;
using fc::sector_storage::stores::AcquireMode;
using fc::sector_storage::stores::HealthReport;
using fc::sector_storage::stores::kMetaFileName;
using fc::sector_storage::stores::LocalPath;
using fc::sector_storage::stores::LocalStorageMock;
using fc::sector_storage::stores::LocalStore;
using fc::sector_storage::stores::LocalStoreImpl;
using fc::sector_storage::stores::PathType;
using fc::sector_storage::stores::SectorIndexMock;
using fc::sector_storage::stores::StorageConfig;
using fc::sector_storage::stores::StorageInfo;
using fc::sector_storage::stores::StoreErrors;
using libp2p::protocol::Scheduler;
using libp2p::protocol::SchedulerMock;
using libp2p::protocol::scheduler::Ticks;
using libp2p::protocol::scheduler::toTicks;
using testing::_;
using testing::Eq;

void createMetaFile(const std::string &storage_path,
                    const LocalStorageMeta &meta) {
  boost::filesystem::path file(storage_path);
  file /= kMetaFileName;
  OUTCOME_EXCEPT(text, fc::codec::json::format(fc::api::encode(meta)));
  OUTCOME_EXCEPT(fc::common::writeFile(file.string(), text));
}

class LocalStoreTest : public test::BaseFS_Test {
 public:
  LocalStoreTest() : test::BaseFS_Test("fc_local_store_test") {
    index_ = std::make_shared<SectorIndexMock>();
    storage_ = std::make_shared<LocalStorageMock>();
    urls_ = {"http://url1.com", "http://url2.com"};
    scheduler_ = std::make_shared<SchedulerMock>();
    EXPECT_CALL(*storage_, getStorage())
        .WillOnce(testing::Return(fc::outcome::success(
            StorageConfig{.storage_paths = std::vector<LocalPath>({})})));

    EXPECT_CALL(*storage_, setStorage(_))
        .WillRepeatedly(testing::Return(fc::outcome::success()));

    current_time_ = toTicks(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()));
    EXPECT_CALL(*scheduler_, now()).WillOnce(testing::Return(current_time_));
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
        .WillOnce(testing::Return(fc::outcome::success(stat)));

    EXPECT_CALL(*index_, storageAttach(storage_info, stat))
        .WillOnce(testing::Return(fc::outcome::success()));

    EXPECT_OUTCOME_TRUE_1(local_store_->openPath(path));
  }

 protected:
  std::shared_ptr<LocalStore> local_store_;
  std::shared_ptr<SectorIndexMock> index_;
  std::shared_ptr<LocalStorageMock> storage_;
  std::vector<std::string> urls_;
  std::shared_ptr<SchedulerMock> scheduler_;
  Ticks current_time_;
};

/**
 * @given sector and registered proof type and sector file types
 * @when try to acquire sector with intersation in exisiting and allocate types
 * @then StoreErrors::FindAndAllocate error occurs
 */
TEST_F(LocalStoreTest, AcquireSectorFindAndAllocate) {
  SectorId sector{
      .miner = 42,
      .sector = 1,
  };

  RegisteredProof seal_proof_type = RegisteredProof::StackedDRG2KiBSeal;

  auto file_type_allocate = static_cast<SectorFileType>(
      SectorFileType::FTCache | SectorFileType::FTUnsealed);

  auto file_type_existing = static_cast<SectorFileType>(
      SectorFileType::FTCache | SectorFileType::FTSealed);

  EXPECT_OUTCOME_ERROR(StoreErrors::kFindAndAllocate,
                       local_store_->acquireSector(sector,
                                                   seal_proof_type,
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
  SectorId sector{
      .miner = 42,
      .sector = 1,
  };

  RegisteredProof seal_proof_type = RegisteredProof::StackedDRG2KiBSeal;

  std::vector res = {StorageInfo{
      .id = "not_found_id",
      .urls = {},
      .weight = 0,
      .can_seal = false,
      .can_store = false,
  }};

  EXPECT_CALL(*index_,
              storageBestAlloc(SectorFileType::FTCache, seal_proof_type, false))
      .WillOnce(testing::Return(fc::outcome::success(res)));

  EXPECT_OUTCOME_ERROR(StoreErrors::kNotFoundPath,
                       local_store_->acquireSector(sector,
                                                   seal_proof_type,
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
  SectorId sector{
      .miner = 42,
      .sector = 1,
  };

  RegisteredProof seal_proof_type = RegisteredProof::StackedDRG2KiBSeal;

  SectorFileType file_type = SectorFileType::FTCache;

  auto storage_path = boost::filesystem::unique_path(
                          fs::canonical(base_path).append("%%%%%-storage"))
                          .string();

  StorageID storage_id = "storage_id";

  fc::primitives::LocalStorageMeta storage_meta{
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

  EXPECT_CALL(*index_, storageBestAlloc(file_type, seal_proof_type, false))
      .WillOnce(testing::Return(fc::outcome::success(res)));

  EXPECT_OUTCOME_TRUE(sectors,
                      local_store_->acquireSector(sector,
                                                  seal_proof_type,
                                                  SectorFileType::FTNone,
                                                  file_type,
                                                  PathType::kStorage,
                                                  AcquireMode::kCopy));

  std::string res_path =
      (boost::filesystem::path(storage_path) / toString(file_type)
       / fc::primitives::sector_file::sectorName(sector))
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
  SectorId sector{
      .miner = 42,
      .sector = 1,
  };

  RegisteredProof seal_proof_type = RegisteredProof::StackedDRG2KiBSeal;

  SectorFileType file_type = SectorFileType::FTCache;

  auto storage_path = boost::filesystem::unique_path(
                          fs::canonical(base_path).append("%%%%%-storage"))
                          .string();

  StorageID storage_id = "storage_id";

  fc::primitives::LocalStorageMeta storage_meta{
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

  EXPECT_CALL(*index_, storageFindSector(sector, file_type, Eq(boost::none)))
      .WillOnce(testing::Return(fc::outcome::success(res)));

  EXPECT_OUTCOME_TRUE(sectors,
                      local_store_->acquireSector(sector,
                                                  seal_proof_type,
                                                  file_type,
                                                  SectorFileType::FTNone,
                                                  PathType::kStorage,
                                                  AcquireMode::kCopy));

  std::string res_path =
      (boost::filesystem::path(storage_path) / toString(file_type)
       / fc::primitives::sector_file::sectorName(sector))
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
  EXPECT_OUTCOME_ERROR(StoreErrors::kNotFoundStorage,
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

  fc::primitives::LocalStorageMeta storage_meta{
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
      .WillOnce(testing::Return(fc::outcome::success(res_stat)));

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

  fc::primitives::LocalStorageMeta storage_meta{
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
                             / fc::primitives::sector_file::sectorName(sector))
                                .string();

  std::ofstream(sector_file).close();

  EXPECT_CALL(*storage_, getStat(storage_path.string()))
      .WillOnce(testing::Return(fc::outcome::success(stat)));

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
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(*index_,
              storageDeclareSector(storage_id, sector, file_type, true))
      .WillOnce(testing::Return(fc::outcome::success()));

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

  fc::primitives::LocalStorageMeta storage_meta{
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
      .WillOnce(testing::Return(fc::outcome::success(stat)));

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
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_ERROR(StoreErrors::kInvalidSectorName,
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

  fc::primitives::LocalStorageMeta storage_meta{
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

  EXPECT_OUTCOME_ERROR(StoreErrors::kDuplicateStorage,
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

  EXPECT_OUTCOME_ERROR(StoreErrors::kRemoveSeveralFileTypes,
                       local_store_->remove(sector, type));

  EXPECT_OUTCOME_ERROR(StoreErrors::kRemoveSeveralFileTypes,
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
          testing::Return(fc::outcome::success(std::vector<StorageInfo>())));

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

  fc::primitives::LocalStorageMeta storage_meta{
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
                             / fc::primitives::sector_file::sectorName(sector))
                                .string();

  std::ofstream(sector_file).close();

  EXPECT_CALL(*storage_, getStat(storage_path.string()))
      .WillOnce(testing::Return(fc::outcome::success(stat)));

  StorageInfo info{
      .id = storage_id,
      .urls = urls_,
      .weight = 0,
      .can_seal = true,
      .can_store = true,
  };

  EXPECT_CALL(*index_, storageAttach(info, stat))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(*index_,
              storageDeclareSector(storage_id, sector, file_type, true))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path.string()));

  std::vector<StorageInfo> res = {info};

  EXPECT_CALL(*index_, storageFindSector(sector, file_type, Eq(boost::none)))
      .WillOnce(testing::Return(fc::outcome::success(res)));

  EXPECT_CALL(*index_, storageDropSector(storage_id, sector, file_type))
      .WillOnce(testing::Return(fc::outcome::success()));

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

  RegisteredProof seal_proof_type = RegisteredProof::StackedDRG2KiBSeal;

  boost::filesystem::create_directories((storage_path / toString(file_type)));

  StorageID storage_id = "storage_id";

  fc::primitives::LocalStorageMeta storage_meta{
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
                             / fc::primitives::sector_file::sectorName(sector))
                                .string();

  std::ofstream(sector_file).close();

  EXPECT_CALL(*storage_, getStat(storage_path.string()))
      .WillOnce(testing::Return(fc::outcome::success(stat)));

  StorageInfo info{
      .id = storage_id,
      .urls = urls_,
      .weight = 0,
      .can_seal = true,
      .can_store = false,
  };

  EXPECT_CALL(*index_, storageAttach(info, stat))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(*index_,
              storageDeclareSector(storage_id, sector, file_type, false))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_OUTCOME_TRUE_1(local_store_->openPath(storage_path.string()));

  auto storage_path_2 = boost::filesystem::unique_path(
      fs::canonical(base_path).append("%%%%%-storage"));

  StorageID storage_id2 = "storage_id2";

  fc::primitives::LocalStorageMeta storage_meta2{
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
      .WillOnce(testing::Return(fc::outcome::success(info)));

  StorageInfo info2{
      .id = storage_id2,
      .urls = urls_,
      .weight = 0,
      .can_seal = true,
      .can_store = true,
  };

  EXPECT_CALL(*index_, getStorageInfo(storage_id2))
      .WillOnce(testing::Return(fc::outcome::success(info2)));

  EXPECT_CALL(*index_, storageDropSector(storage_id, sector, file_type))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(*index_,
              storageDeclareSector(storage_id2, sector, file_type, true))
      .WillOnce(testing::Return(fc::outcome::success()));

  std::string moved_sector_file =
      (storage_path_2 / toString(file_type)
       / fc::primitives::sector_file::sectorName(sector))
          .string();

  ASSERT_FALSE(boost::filesystem::exists(moved_sector_file));
  ASSERT_TRUE(boost::filesystem::exists(sector_file));

  EXPECT_CALL(*index_, storageFindSector(sector, file_type, Eq(boost::none)))
      .WillOnce(testing::Return(fc::outcome::success(std::vector({info}))));

  EXPECT_CALL(*index_, storageBestAlloc(file_type, seal_proof_type, false))
      .WillOnce(testing::Return(fc::outcome::success(std::vector({info2}))));

  EXPECT_OUTCOME_TRUE_1(
      local_store_->moveStorage(sector, seal_proof_type, file_type));

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

  fc::primitives::LocalStorageMeta storage_meta{
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
  EXPECT_CALL(*scheduler_, now())
      .WillOnce(
          testing::Return(current_time_ + toTicks(std::chrono::seconds(12))))
      .WillOnce(
          testing::Return(current_time_ + toTicks(std::chrono::seconds(24))));
  EXPECT_CALL(*index_, storageReportHealth(storage_id, _))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(*storage_, getStat(storage_path))
      .WillOnce(testing::Return(fc::outcome::success(stat)));
  scheduler_->next_clock();
}
