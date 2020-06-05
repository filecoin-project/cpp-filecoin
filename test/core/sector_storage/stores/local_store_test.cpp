/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/stores/store.hpp"

#include <gtest/gtest.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include "api/rpc/json.hpp"
#include "common/outcome.hpp"
#include "sector_storage/stores/impl/local_store.hpp"
#include "sector_storage/stores/store_error.hpp"
#include "testutil/mocks/sector_storage/stores/local_storage_mock.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::primitives::FsStat;
using fc::primitives::LocalStorageMeta;
using fc::primitives::StorageID;
using fc::primitives::sector_file::SectorFileType;
using fc::sector_storage::stores::kMetaFileName;
using fc::sector_storage::stores::LocalStorageMock;
using fc::sector_storage::stores::LocalStore;
using fc::sector_storage::stores::SectorIndexMock;
using fc::sector_storage::stores::StorageInfo;
using fc::sector_storage::stores::StoreErrors;
using testing::_;

template <typename T>
void writeToJSON(const T &obj, const std::string &path) {
  auto doc = fc::api::encode(obj);
  ASSERT_FALSE(doc.HasParseError());

  std::ofstream ofs{path};
  ASSERT_TRUE(ofs.is_open());

  rapidjson::OStreamWrapper osw{ofs};
  rapidjson::Writer<rapidjson::OStreamWrapper> writer{osw};
  doc.Accept(writer);
}

void createStorage(const std::string &path, const LocalStorageMeta &meta) {
  boost::filesystem::path file(path);
  file /= kMetaFileName;

  writeToJSON(meta, file.string());
}

class LocalStoreTest : public test::BaseFS_Test {
 public:
  LocalStoreTest() : test::BaseFS_Test("fc_local_store_test") {
    index_ = std::make_shared<SectorIndexMock>();
    storage_ = std::make_shared<LocalStorageMock>();
    urls_ = {"http://url1.com", "http://url2.com"};

    storage_path1_ = boost::filesystem::unique_path(
                         fs::canonical(base_path).append("%%%%%-storage"))
                         .string();
    boost::filesystem::create_directory(storage_path1_);

    storage_1 = "storage_1";

    fc::primitives::LocalStorageMeta storage_meta1{
        .id = storage_1,
        .weight = 0,
        .can_store = true,
        .can_seal = true,
    };

    createStorage(storage_path1_, storage_meta1);

    storage_path2_ = boost::filesystem::unique_path(
                         fs::canonical(base_path).append("%%%%%-storage"))
                         .string();
    boost::filesystem::create_directory(storage_path2_);

    storage_2 = "storage_2";

    fc::primitives::LocalStorageMeta storage_meta2{
        .id = storage_2,
        .weight = 0,
        .can_store = true,
        .can_seal = true,
    };

    createStorage(storage_path2_, storage_meta2);

    std::vector<std::string> paths = {storage_path1_, storage_path2_};

    EXPECT_CALL(*storage_, getStat(storage_path1_))
        .WillOnce(testing::Return(fc::outcome::success(FsStat{
            .capacity = 100,
            .available = 100,
            .used = 0,
        })));

    EXPECT_CALL(*storage_, getStat(storage_path2_))
        .WillOnce(testing::Return(fc::outcome::success(FsStat{
            .capacity = 200,
            .available = 200,
            .used = 0,
        })));

    EXPECT_CALL(*storage_, getPaths())
        .WillOnce(testing::Return(fc::outcome::success(paths)));

    EXPECT_CALL(*index_, storageAttach(_, _))
        .WillRepeatedly(testing::Return(fc::outcome::success()));

    auto maybe_local = LocalStore::newLocalStore(storage_, index_, urls_);
    local_store_ = maybe_local.value();
  }

 protected:
  std::shared_ptr<LocalStore> local_store_;
  std::shared_ptr<SectorIndexMock> index_;
  std::shared_ptr<LocalStorageMock> storage_;
  std::vector<std::string> urls_;
  std::string storage_path1_;
  std::string storage_path2_;
  StorageID storage_1;
  StorageID storage_2;
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

  EXPECT_OUTCOME_ERROR(StoreErrors::NotFoundPath,
                       local_store_->acquireSector(sector,
                                                   seal_proof_type,
                                                   SectorFileType::FTNone,
                                                   SectorFileType::FTCache,
                                                   false))
}

TEST_F(LocalStoreTest, AcqireSectorAllocateSuccess) {
  SectorId sector{
      .miner = 42,
      .sector = 1,
  };

  RegisteredProof seal_proof_type = RegisteredProof::StackedDRG2KiBSeal;

  SectorFileType file_type = SectorFileType::FTCache;

  std::vector res = {StorageInfo{
                         .id = storage_1,
                         .urls = urls_,
                         .weight = 0,
                         .can_seal = true,
                         .can_store = true,
                     },
                     StorageInfo{
                         .id = storage_2,
                         .urls = urls_,
                         .weight = 0,
                         .can_seal = true,
                         .can_store = true,
                     }};

  EXPECT_CALL(*index_, storageBestAlloc(file_type, seal_proof_type, false))
      .WillOnce(testing::Return(fc::outcome::success(res)));

  EXPECT_OUTCOME_TRUE(
      sectors,
      local_store_->acquireSector(
          sector, seal_proof_type, SectorFileType::FTNone, file_type, false));

  std::string res_path =
      (boost::filesystem::path(storage_path1_) / toString(file_type)
       / fc::primitives::sector_file::sectorName(sector))
          .string();

  EXPECT_OUTCOME_EQ(sectors.paths.getPathByType(file_type), res_path);
  EXPECT_OUTCOME_EQ(sectors.stores.getPathByType(file_type), storage_1);
}

TEST_F(LocalStoreTest, AcqireSectorExistSuccess) {
  SectorId sector{
      .miner = 42,
      .sector = 1,
  };

  RegisteredProof seal_proof_type = RegisteredProof::StackedDRG2KiBSeal;

  SectorFileType file_type = SectorFileType::FTCache;

  std::vector res = {StorageInfo{
                         .id = storage_2,
                         .urls = urls_,
                         .weight = 0,
                         .can_seal = true,
                         .can_store = true,
                     }};

  EXPECT_CALL(*index_, storageFindSector(sector, file_type, false))
      .WillOnce(testing::Return(fc::outcome::success(res)));

  EXPECT_OUTCOME_TRUE(
      sectors,
      local_store_->acquireSector(
          sector, seal_proof_type, file_type, SectorFileType::FTNone, false));

  std::string res_path =
      (boost::filesystem::path(storage_path2_) / toString(file_type)
       / fc::primitives::sector_file::sectorName(sector))
          .string();

  EXPECT_OUTCOME_EQ(sectors.paths.getPathByType(file_type), res_path);
  EXPECT_OUTCOME_EQ(sectors.stores.getPathByType(file_type), storage_2);
}
