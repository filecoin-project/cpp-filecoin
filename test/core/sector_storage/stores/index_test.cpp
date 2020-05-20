/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "sector_storage/stores/impl/index_impl.hpp"

#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::sector_storage::stores::FsStat;
using fc::sector_storage::stores::IndexErrors;
using fc::sector_storage::stores::SectorIndexImpl;
using fc::sector_storage::stores::StorageInfo;

class SectorIndexTest : public test::BaseFS_Test {
 public:
  SectorIndexTest() : test::BaseFS_Test("fc_sector_index_test") {
    sector_index_ = std::make_unique<SectorIndexImpl>();
  }

 protected:
  std::unique_ptr<SectorIndexImpl> sector_index_;
};

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
  };
  FsStat file_system_stat{};

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
  };
  FsStat file_system_stat{};

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
