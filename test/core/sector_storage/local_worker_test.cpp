/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/local_worker.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <random>
#include "proofs/proof_param_provider.hpp"
#include "proofs/proofs.hpp"
#include "sector_storage/stores/store_error.hpp"
#include "testutil/mocks/sector_storage/stores/local_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/remote_store_mock.hpp"
#include "testutil/mocks/sector_storage/stores/sector_index_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::primitives::StoragePath;
using fc::primitives::piece::PieceData;
using fc::primitives::piece::UnpaddedPieceSize;
using fc::primitives::sector::InteractiveRandomness;
using fc::primitives::sector::OnChainSealVerifyInfo;
using fc::proofs::PieceInfo;
using fc::proofs::ProofParamProvider;
using fc::proofs::SealVerifyInfo;
using fc::sector_storage::LocalWorker;
using fc::sector_storage::WorkerConfig;
using fc::sector_storage::stores::LocalStoreMock;
using fc::sector_storage::stores::RemoteStoreMock;
using fc::sector_storage::stores::SectorIndexMock;
using proofs = fc::proofs::Proofs;
using fc::primitives::sector_file::SectorFileType;
using fc::sector_storage::stores::AcquireSectorResponse;

class LocalWorkerTest : public test::BaseFS_Test {
 public:
  LocalWorkerTest() : test::BaseFS_Test("fc_local_worker_test") {
    tasks_ = {
        fc::primitives::kTTAddPiece,
        fc::primitives::kTTPreCommit1,
        fc::primitives::kTTPreCommit2,
    };
    seal_proof_type_ = RegisteredProof::StackedDRG2KiBSeal;
    worker_name_ = "local worker";

    config_ = WorkerConfig{.hostname = worker_name_,
                           .seal_proof_type = seal_proof_type_,
                           .task_types = tasks_};
    store_ = std::make_shared<RemoteStoreMock>();
    local_store_ = std::make_shared<LocalStoreMock>();
    sector_index_ = std::make_shared<SectorIndexMock>();

    EXPECT_CALL(*store_, getLocalStore())
        .WillRepeatedly(testing::Return(local_store_));

    EXPECT_CALL(*store_, getSectorIndex())
        .WillRepeatedly(testing::Return(sector_index_));

    EXPECT_CALL(*local_store_, getSectorIndex())
        .WillRepeatedly(testing::Return(sector_index_));

    local_worker_ = std::make_unique<LocalWorker>(config_, store_);
  }

 protected:
  std::set<fc::primitives::TaskType> tasks_;
  RegisteredProof seal_proof_type_;
  std::string worker_name_;
  WorkerConfig config_;
  std::shared_ptr<RemoteStoreMock> store_;
  std::shared_ptr<LocalStoreMock> local_store_;
  std::shared_ptr<SectorIndexMock> sector_index_;
  std::unique_ptr<LocalWorker> local_worker_;
};

/**
 * @given local worker
 * @when when try to getSupportedTask
 * @then gets supported tasks for the worker
 */
TEST_F(LocalWorkerTest, getSupportedTask) {
  EXPECT_OUTCOME_EQ(local_worker_->getSupportedTask(), tasks_);
}

/**
 * @given local worker
 * @when when try to getInfo
 * @then gets info about the worker
 */
TEST_F(LocalWorkerTest, getInfo) {
  EXPECT_OUTCOME_TRUE(info, local_worker_->getInfo());
  ASSERT_EQ(info.hostname, worker_name_);
}

/**
 * @given local worker
 * @when when try to getAccessiblePaths
 * @then gets paths that accessible by the worker
 */
TEST_F(LocalWorkerTest, getAccessiblePaths) {
  std::vector<StoragePath> paths = {StoragePath{
                                        .id = "id1",
                                        .weight = 10,
                                        .local_path = "/some/path/1",
                                        .can_seal = false,
                                        .can_store = true,
                                    },
                                    StoragePath{
                                        .id = "id2",
                                        .weight = 100,
                                        .local_path = "/some/path/2",
                                        .can_seal = true,
                                        .can_store = false,
                                    }};
  EXPECT_CALL(*local_store_, getAccessiblePaths())
      .WillOnce(testing::Return(fc::outcome::success(paths)));
  EXPECT_OUTCOME_EQ(local_worker_->getAccessiblePaths(), paths);
}

/**
 * @given sector
 * @when when try to preSealCommit with sum of sizes of pieces and not equal to
 * sector size
 * @then Error occurs
 */
TEST_F(LocalWorkerTest, PreCommit_MatchSumError) {
  SectorId sector{
      .miner = 1,
      .sector = 3,
  };

  fc::proofs::SealRandomness ticket{{5, 4, 2}};

  AcquireSectorResponse response{};

  response.paths.sealed = (base_path / toString(SectorFileType::FTSealed)
                           / fc::primitives::sector_file::sectorName(sector))
                              .string();
  response.paths.cache = (base_path / toString(SectorFileType::FTCache)
                          / fc::primitives::sector_file::sectorName(sector))
                             .string();

  EXPECT_CALL(*store_, remove(sector, SectorFileType::FTSealed))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(*store_, remove(sector, SectorFileType::FTCache))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(*sector_index_,
              storageDeclareSector(
                  response.storages.cache, sector, SectorFileType::FTCache))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(*sector_index_,
              storageDeclareSector(
                  response.storages.sealed, sector, SectorFileType::FTSealed))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(
      *store_,
      acquireSector(sector,
                    config_.seal_proof_type,
                    SectorFileType::FTUnsealed,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    true))
      .WillOnce(testing::Return(fc::outcome::success(response)));

  ASSERT_TRUE(boost::filesystem::create_directory(
      base_path / toString(SectorFileType::FTSealed)));
  ASSERT_TRUE(boost::filesystem::create_directory(
      base_path / toString(SectorFileType::FTCache)));

  EXPECT_OUTCOME_ERROR(
      fc::sector_storage::WorkerErrors::kPiecesDoNotMatchSectorSize,
      local_worker_->sealPreCommit1(sector, ticket, {}));
}

/**
 * @given pieces
 * @when try to add Pieces, Seal, Unseal, ReadPiece and compare with original
 * @then Success
 */
TEST_F(LocalWorkerTest, Sealer) {
  EXPECT_OUTCOME_TRUE(
      params,
      ProofParamProvider::readJson(
          "/var/tmp/filecoin-proof-parameters/parameters.json"));
  EXPECT_OUTCOME_TRUE(sector_size,
                      fc::primitives::sector::getSectorSize(seal_proof_type_));
  EXPECT_OUTCOME_TRUE_1(
      fc::proofs::ProofParamProvider::getParams(params, sector_size));

  fc::common::Blob<2032> some_bytes;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> dis(0, 255);
  for (size_t i = 0; i < some_bytes.size(); i++) {
    some_bytes[i] = dis(gen);
  }

  auto path_model = fs::canonical(base_path).append("%%%%%");
  auto piece_file_a_path = boost::filesystem::unique_path(path_model).string();
  boost::filesystem::ofstream piece_file_a(piece_file_a_path);

  UnpaddedPieceSize piece_commitment_a_size(1016);
  for (size_t i = 0; i < piece_commitment_a_size; i++) {
    piece_file_a << some_bytes[i];
  }
  piece_file_a.close();

  auto piece_file_b_path = boost::filesystem::unique_path(path_model).string();
  boost::filesystem::ofstream piece_file_b(piece_file_b_path);
  UnpaddedPieceSize piece_commitment_b_size(1016);
  for (size_t i = 0; i < piece_commitment_b_size; i++) {
    piece_file_b << some_bytes[i];
  }
  piece_file_b.close();

  PieceData file_a(piece_file_a_path);
  PieceData file_b(piece_file_b_path);

  SectorId sector{
      .miner = 1,
      .sector = 3,
  };

  AcquireSectorResponse response{};

  response.paths.unsealed = (base_path / toString(SectorFileType::FTUnsealed)
                             / fc::primitives::sector_file::sectorName(sector))
                                .string();
  response.storages.unsealed = "some-id";

  ASSERT_TRUE(boost::filesystem::create_directory(
      base_path / toString(SectorFileType::FTUnsealed)));

  EXPECT_CALL(*store_,
              acquireSector(sector,
                            seal_proof_type_,
                            SectorFileType::FTNone,
                            SectorFileType::FTUnsealed,
                            true))
      .WillOnce(testing::Return(fc::outcome::success(response)));
  EXPECT_CALL(
      *sector_index_,
      storageDeclareSector(
          response.storages.unsealed, sector, SectorFileType::FTUnsealed))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_OUTCOME_TRUE(
      a_info,
      local_worker_->addPiece(sector, {}, piece_commitment_a_size, file_a));

  EXPECT_CALL(*store_,
              acquireSector(sector,
                            seal_proof_type_,
                            SectorFileType::FTUnsealed,
                            SectorFileType::FTNone,
                            true))
      .WillOnce(testing::Return(fc::outcome::success(response)));

  EXPECT_OUTCOME_TRUE(b_info,
                      local_worker_->addPiece(sector,
                                              {{piece_commitment_a_size}},
                                              piece_commitment_b_size,
                                              file_b));

  fc::proofs::SealRandomness ticket{{5, 4, 2}};

  std::vector<PieceInfo> pieces = {a_info, b_info};

  EXPECT_CALL(*store_, remove(sector, SectorFileType::FTSealed))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(*store_, remove(sector, SectorFileType::FTCache))
      .WillOnce(testing::Return(fc::outcome::success()));

  response.paths.sealed = (base_path / toString(SectorFileType::FTSealed)
                           / fc::primitives::sector_file::sectorName(sector))
                              .string();
  response.storages.sealed = "some-id";

  ASSERT_TRUE(boost::filesystem::create_directory(
      base_path / toString(SectorFileType::FTSealed)));

  response.paths.cache = (base_path / toString(SectorFileType::FTCache)
                          / fc::primitives::sector_file::sectorName(sector))
                             .string();
  response.storages.cache = "some-id";

  ASSERT_TRUE(boost::filesystem::create_directory(
      base_path / toString(SectorFileType::FTCache)));

  EXPECT_CALL(*sector_index_,
              storageDeclareSector(
                  response.storages.cache, sector, SectorFileType::FTCache))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(*sector_index_,
              storageDeclareSector(
                  response.storages.sealed, sector, SectorFileType::FTSealed))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(
      *store_,
      acquireSector(sector,
                    config_.seal_proof_type,
                    SectorFileType::FTUnsealed,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    true))
      .WillOnce(testing::Return(fc::outcome::success(response)));

  EXPECT_OUTCOME_TRUE(pc1o,
                      local_worker_->sealPreCommit1(sector, ticket, pieces));

  response.paths.unsealed = "";
  response.storages.unsealed = "";

  EXPECT_CALL(
      *store_,
      acquireSector(sector,
                    config_.seal_proof_type,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTNone,
                    true))
      .WillOnce(testing::Return(fc::outcome::success(response)));

  EXPECT_OUTCOME_TRUE(cids, local_worker_->sealPreCommit2(sector, pc1o));

  InteractiveRandomness seed{{7, 4, 2}};

  EXPECT_CALL(
      *store_,
      acquireSector(sector,
                    config_.seal_proof_type,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTNone,
                    true))
      .WillOnce(testing::Return(fc::outcome::success(response)));

  EXPECT_OUTCOME_TRUE(
      c1o, local_worker_->sealCommit1(sector, ticket, seed, pieces, cids));

  EXPECT_OUTCOME_TRUE(proof, local_worker_->sealCommit2(sector, c1o));

  EXPECT_OUTCOME_TRUE(valid,
                      proofs::verifySeal(SealVerifyInfo{
                          .sector = sector,
                          .info =
                              OnChainSealVerifyInfo{
                                  .sealed_cid = cids.sealed_cid,
                                  .interactive_epoch = 42,
                                  .registered_proof = seal_proof_type_,
                                  .proof = proof,
                                  .deals = {},
                                  .sector = sector.sector,
                                  .seal_rand_epoch = 42,
                              },
                          .randomness = ticket,
                          .interactive_randomness = seed,
                          .unsealed_cid = cids.unsealed_cid,
                      }));

  ASSERT_TRUE(valid);

  auto unseal_path = base_path / toString(SectorFileType::FTUnsealed)
                     / fc::primitives::sector_file::sectorName(sector);

  boost::filesystem::remove(unseal_path.string());

  AcquireSectorResponse unseal_response{};

  unseal_response.paths.unsealed = unseal_path.string();

  EXPECT_CALL(*store_,
              acquireSector(sector,
                            config_.seal_proof_type,
                            SectorFileType::FTUnsealed,
                            SectorFileType::FTNone,
                            false))
      .WillOnce(testing::Return(
          fc::outcome::failure(fc::sector_storage::stores::StoreErrors::
                                   kNotFoundRequestedSectorType)));

  EXPECT_CALL(*store_,
              acquireSector(sector,
                            config_.seal_proof_type,
                            SectorFileType::FTNone,
                            SectorFileType::FTUnsealed,
                            false))
      .WillOnce(testing::Return(fc::outcome::success(unseal_response)));

  EXPECT_CALL(
      *store_,
      acquireSector(sector,
                    config_.seal_proof_type,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache),
                    SectorFileType::FTNone,
                    false))
      .WillOnce(testing::Return(fc::outcome::success(response)));

  EXPECT_OUTCOME_TRUE_1(local_worker_->unsealPiece(
      sector, 0, piece_commitment_a_size, ticket, cids.unsealed_cid));

  int piece[2];
  ASSERT_FALSE(pipe(piece) < 0);

  EXPECT_CALL(*store_,
              acquireSector(sector,
                            config_.seal_proof_type,
                            SectorFileType::FTUnsealed,
                            SectorFileType::FTNone,
                            false))
      .WillOnce(testing::Return(fc::outcome::success(unseal_response)));

  EXPECT_OUTCOME_TRUE_1(local_worker_->readPiece(
      PieceData(piece[1]), sector, 0, piece_commitment_a_size));

  std::ifstream original_a_piece(piece_file_a_path, std::ifstream::binary);

  ASSERT_TRUE(original_a_piece.is_open());

  char original_ch;
  char piece_ch;

  for (uint64_t read_size = 0; read_size < piece_commitment_a_size;
       read_size++) {
    original_a_piece.read(&original_ch, 1);
    read(piece[0], &piece_ch, 1);
    ASSERT_EQ(original_ch, piece_ch);
  }
}

/**
 * @given sector
 * @when try to remove sector
 * @then Success
 */
TEST_F(LocalWorkerTest, Remove_Success) {
  SectorId sector{
      .miner = 1,
      .sector = 3,
  };

  for (const auto &type : fc::primitives::sector_file::kSectorFileTypes) {
    EXPECT_CALL(*store_, remove(sector, type))
        .WillOnce(testing::Return(fc::outcome::success()));
  }

  EXPECT_OUTCOME_TRUE_1(local_worker_->remove(sector));
}

/**
 * @given sector
 * @when try to remove sector and one type removed with error
 * @then error occurs
 */
TEST_F(LocalWorkerTest, Remove_Error) {
  SectorId sector{
      .miner = 1,
      .sector = 3,
  };

  EXPECT_CALL(*store_, remove(sector, SectorFileType::FTUnsealed))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(*store_, remove(sector, SectorFileType::FTSealed))
      .WillOnce(testing::Return(fc::outcome::success()));
  EXPECT_CALL(*store_, remove(sector, SectorFileType::FTCache))
      .WillOnce(testing::Return(fc::outcome::failure(
          fc::sector_storage::stores::StoreErrors::kNotFoundSector)));

  EXPECT_OUTCOME_ERROR(fc::sector_storage::WorkerErrors::kCannotRemoveSector,
                       local_worker_->remove(sector));
}
