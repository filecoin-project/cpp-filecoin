/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "sector_storage/stores/impl/index_impl.hpp"

#include <memory>
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::primitives::sector::RegisteredProof;
using fc::primitives::sector_file::SectorFileType;
using fc::sector_storage::stores::FsStat;
using fc::sector_storage::stores::IndexErrors;
using fc::sector_storage::stores::SectorIndex;
using fc::sector_storage::stores::SectorIndexImpl;
using fc::sector_storage::stores::StorageInfo;

class SectorIndexTest : public test::BaseFS_Test {
 public:
  SectorIndexTest() : test::BaseFS_Test("fc_sector_index_test") {
    sector_index_ = std::make_shared<SectorIndexImpl>();
  }

 protected:
  std::shared_ptr<SectorIndex> sector_index_;
};

bool operator==(const StorageInfo &lhs, const StorageInfo &rhs) {
  return lhs.id == rhs.id && lhs.urls == rhs.urls && lhs.weight == rhs.weight
         && lhs.can_seal == rhs.can_seal && lhs.can_store == rhs.can_store
         && lhs.last_heartbreak == rhs.last_heartbreak
         && lhs.error == rhs.error;
}

/**
 * @given sector
 * @when want to get path to sealed and cache sector
 * @then sealed sector was obtained. cache and unsealed paths are empty
 */
TEST_F(SectorIndexTest, AttachNewStorage) {
  std::string id = "test_id";
  std::vector<std::string> urls;
  urls.emplace_back("url1.com");
  urls.emplace_back("url2.com");
  urls.emplace_back("url3.com");
  StorageInfo storage_info{
      .id = id,
      .urls = urls,
      .weight = 0,
      .can_seal = false,
      .can_store = false,
      .last_heartbreak = std::chrono::system_clock::now(),
      .error = {},
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

TEST_F(SectorIndexTest, AttachExistStorage) {
  std::string id = "test_id";
  std::vector<std::string> urls;
  urls.reserve(5);

  for (int i = 0; i < 5; i++) {
    urls.emplace_back("url" + std::to_string(i) + ".com");
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
      .last_heartbreak = std::chrono::system_clock::now(),
      .error = {},
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

TEST_F(SectorIndexTest, NotFoundStorage) {
  EXPECT_OUTCOME_ERROR(IndexErrors::StorageNotFound,
                       sector_index_->getStorageInfo("not_found_id"));
}

TEST_F(SectorIndexTest, BestAllocationNoSuitableStorage) {
  EXPECT_OUTCOME_ERROR(
      IndexErrors::NoSuitableCandidate,
      sector_index_->storageBestAlloc(
          SectorFileType::FTCache, RegisteredProof::StackedDRG2KiBSeal, false));
}

TEST_F(SectorIndexTest, BestAllocation) {
  std::string id1 = "id1";
  StorageInfo storage_info1{
      .id = id1,
      .urls = {},
      .weight = 10,
      .can_seal = false,
      .can_store = true,
      .last_heartbreak = std::chrono::system_clock::now(),
      .error = {},
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
      .last_heartbreak = std::chrono::system_clock::now(),
      .error = {},
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
      .last_heartbreak = std::chrono::system_clock::now(),
      .error = {},
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
  ASSERT_EQ(candidates.at(0).id, id1);
  ASSERT_EQ(candidates.at(1).id, id3);
}
