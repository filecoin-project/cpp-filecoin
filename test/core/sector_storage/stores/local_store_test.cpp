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

class LocalStoreTest : public test::BaseFS_Test {
 public:
  LocalStoreTest() : test::BaseFS_Test("fc_local_store_test") {
    index_ = std::make_shared<SectorIndexMock>();
    storage_ = std::make_shared<LocalStorageMock>();
    urls_ = {"http://url1.com", "http://url2.com"};

    EXPECT_CALL(*storage_, getPaths())
        .WillOnce(testing::Return(
            fc::outcome::success(std::vector<std::string>({}))));

    auto maybe_local = LocalStore::newLocalStore(storage_, index_, urls_);
    local_store_ = maybe_local.value();
  }

  void createStorage(const std::string &path,
                     const LocalStorageMeta &meta,
                     const FsStat &stat) {
    if (!boost::filesystem::exists(path)) {
      boost::filesystem::create_directory(path);
    }

    boost::filesystem::path file(path);
    file /= kMetaFileName;

    writeToJSON(meta, file.string());

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

  auto storage_path = boost::filesystem::unique_path(
                          fs::canonical(base_path).append("%%%%%-storage"))
                          .string();

  StorageID storage_id = "storage_id";

  fc::primitives::LocalStorageMeta storage_meta{
      .id = storage_id,
      .weight = 0,
      .can_store = true,
      .can_seal = true,
  };

  FsStat stat{
      .capacity = 100,
      .available = 100,
      .used = 0,
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

  EXPECT_OUTCOME_TRUE(
      sectors,
      local_store_->acquireSector(
          sector, seal_proof_type, SectorFileType::FTNone, file_type, false));

  std::string res_path =
      (boost::filesystem::path(storage_path) / toString(file_type)
       / fc::primitives::sector_file::sectorName(sector))
          .string();

  EXPECT_OUTCOME_EQ(sectors.paths.getPathByType(file_type), res_path);
  EXPECT_OUTCOME_EQ(sectors.stores.getPathByType(file_type), storage_id);
}

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
      .can_store = true,
      .can_seal = true,
  };

  FsStat stat{
      .capacity = 200,
      .available = 200,
      .used = 0,
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

  EXPECT_CALL(*index_, storageFindSector(sector, file_type, false))
      .WillOnce(testing::Return(fc::outcome::success(res)));

  EXPECT_OUTCOME_TRUE(
      sectors,
      local_store_->acquireSector(
          sector, seal_proof_type, file_type, SectorFileType::FTNone, false));

  std::string res_path =
      (boost::filesystem::path(storage_path) / toString(file_type)
       / fc::primitives::sector_file::sectorName(sector))
          .string();

  EXPECT_OUTCOME_EQ(sectors.paths.getPathByType(file_type), res_path);
  EXPECT_OUTCOME_EQ(sectors.stores.getPathByType(file_type), storage_id);
}

TEST_F(LocalStoreTest, getFSStatNotFound) {
  EXPECT_OUTCOME_ERROR(StoreErrors::NotFoundStorage,
                       local_store_->getFsStat("not_found_id"));
}

TEST_F(LocalStoreTest, getFSStatSuccess) {
  auto storage_path = boost::filesystem::unique_path(
                          fs::canonical(base_path).append("%%%%%-storage"))
                          .string();

  StorageID storage_id = "storage_id";

  fc::primitives::LocalStorageMeta storage_meta{
      .id = storage_id,
      .weight = 0,
      .can_store = true,
      .can_seal = true,
  };

  FsStat res_stat{
      .capacity = 100,
      .available = 100,
      .used = 0,
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
