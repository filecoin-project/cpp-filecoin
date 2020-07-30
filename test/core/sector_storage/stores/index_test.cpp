/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "sector_storage/stores/impl/index_impl.hpp"

#include <memory>
#include "testutil/outcome.hpp"

using fc::primitives::sector::RegisteredProof;
using fc::primitives::sector_file::SectorFileType;
using fc::sector_storage::stores::FsStat;
using fc::sector_storage::stores::IndexErrors;
using fc::sector_storage::stores::SectorIndex;
using fc::sector_storage::stores::SectorIndexImpl;
using fc::sector_storage::stores::StorageInfo;

class SectorIndexTest : public testing::Test {
 public:
  void SetUp() override {
    sector_index_ = std::make_shared<SectorIndexImpl>();
  }

 protected:
  std::shared_ptr<SectorIndex> sector_index_;
};

/**
 * @given storage info
 * @when try to attach new storage
 * @then storage is in the system
 */
TEST_F(SectorIndexTest, AttachNewStorage) {
  std::string id = "test_id";
  std::vector<std::string> urls;
  urls.emplace_back("http://url1.com");
  urls.emplace_back("http://url2.com");
  urls.emplace_back("https://url3.com");
  StorageInfo storage_info{
      .id = id,
      .urls = urls,
      .weight = 0,
      .can_seal = false,
      .can_store = false,
  };
  FsStat file_system_stat{
      .capacity = 100,
      .available = 100,
      .used = 0,
  };

  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageAttach(storage_info, file_system_stat));
  EXPECT_OUTCOME_TRUE(si, sector_index_->getStorageInfo(id));
  ASSERT_EQ(si.urls, urls);
}

/**
 * @given storage info and extended storage info with same id
 * @when try to attach storage with same id
 * @then url list is extended
 */
TEST_F(SectorIndexTest, AttachExistStorage) {
  std::string id = "test_id";
  std::vector<std::string> urls;
  urls.reserve(5);

  for (int i = 0; i < 5; i++) {
    urls.emplace_back("http://url" + std::to_string(i) + ".com");
  }

  std::vector<std::string> urls1;
  urls1.push_back(urls[0]);
  urls1.push_back(urls[1]);
  urls1.push_back(urls[2]);

  StorageInfo storage_info{
      .id = id,
      .urls = urls1,
      .weight = 0,
      .can_seal = false,
      .can_store = false,
  };
  FsStat file_system_stat{
      .capacity = 100,
      .available = 100,
      .used = 0,
  };

  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageAttach(storage_info, file_system_stat));

  std::vector<std::string> urls2;
  urls2.push_back(urls[2]);
  urls2.push_back(urls[3]);
  urls2.push_back(urls[4]);

  StorageInfo storage_info2{
      .id = id,
      .urls = urls2,
      .weight = 0,
      .can_seal = false,
      .can_store = false,
  };

  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageAttach(storage_info2, file_system_stat));
  EXPECT_OUTCOME_TRUE(si, sector_index_->getStorageInfo(id));

  ASSERT_EQ(si.urls, urls);
}

/**
 * @given storage info with invalid url
 * @when try to attach storage
 * @then get error Invalid Url
 */
TEST_F(SectorIndexTest, AttachStorageWithInvalidUrl) {
  std::string id = "test_id";
  std::vector<std::string> urls;
  urls.emplace_back("http://url1.com");
  urls.emplace_back("http://url2.com");
  urls.emplace_back("invalid_url");
  StorageInfo storage_info{
      .id = id,
      .urls = urls,
      .weight = 0,
      .can_seal = false,
      .can_store = false,
  };
  FsStat file_system_stat{
      .capacity = 100,
      .available = 100,
      .used = 0,
  };

  EXPECT_OUTCOME_ERROR(
      IndexErrors::kInvalidUrl,
      sector_index_->storageAttach(storage_info, file_system_stat));
}

/**
 * @given empty system
 * @when try to find storage
 * @then get error NotFound
 */
TEST_F(SectorIndexTest, NotFoundStorage) {
  EXPECT_OUTCOME_ERROR(IndexErrors::kStorageNotFound,
                       sector_index_->getStorageInfo("not_found_id"));
}

/**
 * @given empty system
 * @when try to find best allocation for file
 * @then get error NoCandidates
 */
TEST_F(SectorIndexTest, BestAllocationNoSuitableStorage) {
  EXPECT_OUTCOME_ERROR(
      IndexErrors::kNoSuitableCandidate,
      sector_index_->storageBestAlloc(
          SectorFileType::FTCache, RegisteredProof::StackedDRG2KiBSeal, false));
}

/**
 * @given 3 storage info
 * @when try to find best allocation for file
 * @then get list satisfactory storages in decreasing order. Second storage is
 * unsatisfactory.
 */
TEST_F(SectorIndexTest, BestAllocation) {
  std::string id1 = "id1";
  StorageInfo storage_info1{
      .id = id1,
      .urls = {},
      .weight = 10,
      .can_seal = false,
      .can_store = true,
  };
  FsStat file_system_stat1{
      .capacity = 7 * 2048,
      .available = 7 * 2048,
      .used = 0,
  };

  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageAttach(storage_info1, file_system_stat1));

  std::string id2 = "id2";
  StorageInfo storage_info2{
      .id = id2,
      .urls = {},
      .weight = 30,
      .can_seal = false,
      .can_store = true,
  };
  FsStat file_system_stat2{
      .capacity = 6 * 2048,
      .available = 6 * 2048,
      .used = 0,
  };

  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageAttach(storage_info2, file_system_stat2));

  std::string id3 = "id3";
  StorageInfo storage_info3{
      .id = id3,
      .urls = {},
      .weight = 5,
      .can_seal = false,
      .can_store = true,
  };
  FsStat file_system_stat3{
      .capacity = 8 * 2048,
      .available = 8 * 2048,
      .used = 0,
  };

  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageAttach(storage_info3, file_system_stat3));

  EXPECT_OUTCOME_TRUE(
      candidates,
      sector_index_->storageBestAlloc(
          SectorFileType::FTCache, RegisteredProof::StackedDRG2KiBSeal, false));

  ASSERT_EQ(candidates.size(), 2);
  ASSERT_EQ(candidates.at(0).id, id3);
  ASSERT_EQ(candidates.at(1).id, id1);
}

/**
 * @given storage info and sector id
 * @when try to add sector to local storage
 * @then sector is successfuly added
 */
TEST_F(SectorIndexTest, StorageDeclareSector) {
  std::string id = "test_id";
  std::vector<std::string> urls;
  urls.emplace_back("http://url1.com/");
  urls.emplace_back("http://url2.com/");
  urls.emplace_back("https://url3.com/");
  StorageInfo storage_info{
      .id = id,
      .urls = urls,
      .weight = 0,
      .can_seal = false,
      .can_store = false,
  };
  FsStat file_system_stat{
      .capacity = 100,
      .available = 100,
      .used = 0,
  };

  SectorId sector{
      .miner = 42,
      .sector = 123,
  };

  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageAttach(storage_info, file_system_stat));
  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageDeclareSector(id, sector, SectorFileType::FTCache));
  EXPECT_OUTCOME_TRUE(
      storages,
      sector_index_->storageFindSector(sector, SectorFileType::FTCache, false));
  ASSERT_EQ(storages.size(), 1);
  ASSERT_EQ(storages[0].id, id);
}

/**
 * @given local storage with sector and sector id
 * @when try to drop sector
 * @then sector is dropped
 */
TEST_F(SectorIndexTest, StorageDropSector) {
  std::string id = "test_id";
  std::vector<std::string> urls;
  urls.emplace_back("http://url1.com/");
  urls.emplace_back("http://url2.com/");
  urls.emplace_back("https://url3.com/");
  StorageInfo storage_info{
      .id = id,
      .urls = urls,
      .weight = 0,
      .can_seal = false,
      .can_store = false,
  };
  FsStat file_system_stat{
      .capacity = 100,
      .available = 100,
      .used = 0,
  };

  SectorId sector{
      .miner = 42,
      .sector = 123,
  };

  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageAttach(storage_info, file_system_stat));
  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageDeclareSector(id, sector, SectorFileType::FTCache));
  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageDropSector(id, sector, SectorFileType::FTCache));
  EXPECT_OUTCOME_TRUE(
      storages,
      sector_index_->storageFindSector(sector, SectorFileType::FTCache, false));
  ASSERT_TRUE(storages.empty());
}

/**
 * @given storage info and sector id
 * @when try to find sector wihout fetch flag
 * @then get storage info from all local storages
 */
TEST_F(SectorIndexTest, StorageFindSector) {
  std::string id = "test_id";
  std::string result_url = "http://url1.com/cache/s-t042-123";
  std::vector<std::string> urls;
  urls.emplace_back("http://url1.com/");
  StorageInfo storage_info{
      .id = id,
      .urls = urls,
      .weight = 0,
      .can_seal = false,
      .can_store = false,
  };
  FsStat file_system_stat{
      .capacity = 100,
      .available = 100,
      .used = 0,
  };

  SectorId sector{
      .miner = 42,
      .sector = 123,
  };

  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageAttach(storage_info, file_system_stat));
  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageDeclareSector(id, sector, SectorFileType::FTCache));
  EXPECT_OUTCOME_TRUE(
      storages,
      sector_index_->storageFindSector(sector, SectorFileType::FTCache, false));
  ASSERT_FALSE(storages.empty());
  auto store = storages[0];
  ASSERT_FALSE(store.urls.empty());
  ASSERT_EQ(store.urls[0], result_url);
}

/**
 * @given storage info and sector id
 * @when try to find sector with fetch flag
 * @then get storage info from all storages
 */
TEST_F(SectorIndexTest, StorageFindSectorFetch) {
  std::string id = "test_id";
  std::string result_url = "http://url1.com/cache/s-t042-123";
  std::vector<std::string> urls;
  urls.emplace_back("http://url1.com/");
  StorageInfo storage_info{
      .id = id,
      .urls = urls,
      .weight = 0,
      .can_seal = false,
      .can_store = false,
  };
  FsStat file_system_stat{
      .capacity = 100,
      .available = 100,
      .used = 0,
  };

  SectorId sector{
      .miner = 42,
      .sector = 123,
  };

  EXPECT_OUTCOME_TRUE_1(
      sector_index_->storageAttach(storage_info, file_system_stat));
  EXPECT_OUTCOME_TRUE(
      storages,
      sector_index_->storageFindSector(sector, SectorFileType::FTCache, true));
  ASSERT_FALSE(storages.empty());
  auto store = storages[0];
  ASSERT_FALSE(store.urls.empty());
  ASSERT_EQ(store.urls[0], result_url);
}

/**
 * @given Sector
 * @when try to lock with waiting and lock again without waiting
 * @then first attempt is success and second is failed
 */
TEST_F(SectorIndexTest, LockSector) {
  SectorId sector{
      .miner = 42,
      .sector = 123,
  };

  SectorFileType read = SectorFileType::FTSealed;
  SectorFileType write = SectorFileType::FTUnsealed;

  EXPECT_OUTCOME_TRUE(lock, sector_index_->storageLock(sector, read, write))
  ASSERT_FALSE(sector_index_->storageTryLock(sector, read, write));
}

/**
 * @given Sector
 * @when try to lock for reading and lock again for reading
 * @then both attempts are successful
 */
TEST_F(SectorIndexTest, LockSectorReading) {
  SectorId sector{
      .miner = 42,
      .sector = 123,
  };

  SectorFileType read = SectorFileType::FTSealed;
  SectorFileType write = SectorFileType::FTNone;

  EXPECT_OUTCOME_TRUE(lock, sector_index_->storageLock(sector, read, write))
  EXPECT_OUTCOME_TRUE(lock1, sector_index_->storageLock(sector, read, write))
}
