/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/store.hpp"

#include <gtest/gtest.h>
#include "common/outcome.hpp"
#include "sector_storage/stores/impl/local_store.hpp"
#include "sector_storage/stores/store_error.hpp"
#include "testutil/mocks/sector_storage/stores/local_storage_mock.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::primitives::sector_file::SectorFileType;
using fc::sector_storage::stores::LocalStorageMock;
using fc::sector_storage::stores::LocalStore;
using fc::sector_storage::stores::SectorIndexMock;
using fc::sector_storage::stores::StorageInfo;
using fc::sector_storage::stores::StoreErrors;

class LocalStoreTest : public test::BaseFS_Test {
 public:
  LocalStoreTest() : test::BaseFS_Test("fc_local_store_test") {
    index_ = std::make_shared<SectorIndexMock>();
    storage_ = std::make_shared<LocalStorageMock>();
    urls_ = {"http://url1.com", "http://url2.com"};
    std::vector<std::string> paths = {storage_path1_, storage_path2_};

    EXPECT_CALL(*storage_, getPaths())
        .WillOnce(testing::Return(fc::outcome::success(paths)));

    auto maybe_local = LocalStore::newLocalStore(storage_, index_, urls_);
    if (maybe_local.has_error()) {
      local_store_ = nullptr;
    } else {
      local_store_ = maybe_local.value();
    }
  }

 protected:
  std::shared_ptr<LocalStore> local_store_;
  std::shared_ptr<SectorIndexMock> index_;
  std::shared_ptr<LocalStorageMock> storage_;
  std::vector<std::string> urls_;
  std::string storage_path1_;
  std::string storage_path2_;
};

TEST_F(LocalStoreTest, AcqireSectorFindAndAllocate) {
  SectorId sector{
      .miner = 42,
      .sector = 1,
  };

  RegisteredProof seal_proof_type = RegisteredProof::StackedDRG2KiBSeal;

  EXPECT_OUTCOME_ERROR(StoreErrors::FindAndAllocate,
                       local_store_->acquireSector(sector,
                                                   seal_proof_type,
                                                   SectorFileType::FTCache,
                                                   SectorFileType::FTCache,
                                                   false))
}

TEST_F(LocalStoreTest, AcqireSectorNotFoundPath) {
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

  EXPECT_OUTCOME_ERROR(
      StoreErrors::NotFoundPath,
      local_store_->acquireSector(sector,
                                  seal_proof_type,

                                  static_cast<SectorFileType>(0),
                                  SectorFileType::FTCache,
                                  false))
}
