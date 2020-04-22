/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include "sector_storage/impl/sector_storage_impl.hpp"
#include "sector_storage/sector_storage.hpp"
#include "sector_storage/sector_storage_error.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::primitives::sector_file::SectorFileTypes;
using fc::sector_storage::SectorStorage;
using fc::sector_storage::SectorStorageError;
using fc::sector_storage::SectorStorageImpl;

class SectorStorageTest : public test::BaseFS_Test {
 public:
  SectorStorageTest() : test::BaseFS_Test("fc_sector_storage_test") {
    sector_storage_ =
        std::make_unique<SectorStorageImpl>(base_path.string(),
                                            RegisteredProof::StackedDRG2KiBPoSt,
                                            RegisteredProof::StackedDRG2KiBSeal,
                                            100);
  }

 protected:
  std::unique_ptr<SectorStorage> sector_storage_;
};

TEST_F(SectorStorageTest, AcquireSector_Sealed) {
  std::string sealed_result =
      (base_path / fs::path("sealed") / fs::path("s-t01-3")).string();

  SectorId sector{
      .sector = 3,
      .miner = 1,
  };

  EXPECT_OUTCOME_TRUE(paths,
                      sector_storage_->acquireSector(
                          sector, SectorFileTypes::FTSealed, 0, true))

  ASSERT_TRUE(paths.cache.empty());
  ASSERT_TRUE(paths.unsealed.empty());
  ASSERT_EQ(paths.sealed, sealed_result);
}

TEST_F(SectorStorageTest, AcquireSector_Unsealed) {
  std::string cache_result = "";
  std::string unsealed_result =
      (base_path / fs::path("unsealed") / fs::path("s-t01-3")).string();
  std::string sealed_result = "";

  SectorId sector{
      .sector = 3,
      .miner = 1,
  };

  EXPECT_OUTCOME_TRUE(paths,
                      sector_storage_->acquireSector(
                          sector, SectorFileTypes::FTUnsealed, 0, true))

  ASSERT_TRUE(paths.cache.empty());
  ASSERT_EQ(paths.unsealed, unsealed_result);
  ASSERT_TRUE(paths.sealed.empty());
}

TEST_F(SectorStorageTest, AcquireSector_Cache) {
  std::string cache_result =
      (base_path / fs::path("cache") / fs::path("s-t01-3")).string();
  std::string unsealed_result =
      (base_path / fs::path("unsealed") / fs::path("s-t01-3")).string();
  std::string sealed_result =
      (base_path / fs::path("sealed") / fs::path("s-t01-3")).string();

  SectorId sector{
      .sector = 3,
      .miner = 1,
  };

  EXPECT_OUTCOME_TRUE(
      paths,
      sector_storage_->acquireSector(sector, SectorFileTypes::FTCache, 0, true))

  ASSERT_EQ(paths.cache, cache_result);
  ASSERT_EQ(paths.unsealed, unsealed_result);
  ASSERT_EQ(paths.sealed, sealed_result);
}
