/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <random>
#include <storage/filestore/path.hpp>

#include "sector_storage/impl/sector_storage_impl.hpp"
#include "sector_storage/sector_storage.hpp"
#include "sector_storage/sector_storage_error.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::primitives::sector::OnChainSealVerifyInfo;
using fc::primitives::sector::SealVerifyInfo;
using fc::primitives::sector_file::SectorFileTypes;
using fc::proofs::PieceData;
using fc::proofs::PieceInfo;
using fc::proofs::UnpaddedPieceSize;
using fc::sector_storage::InteractiveRandomness;
using fc::sector_storage::SectorStorage;
using fc::sector_storage::SectorStorageError;
using fc::sector_storage::SectorStorageImpl;
using fc::storage::filestore::Path;
using proofs = fc::proofs::Proofs;

class SectorStorageTest : public test::BaseFS_Test {
 public:
  SectorStorageTest() : test::BaseFS_Test("fc_sector_storage_test") {
    sector_storage_ = std::make_unique<SectorStorageImpl>(
        base_path.string(), post_proof_, seal_proof_);
  }

 protected:
  RegisteredProof seal_proof_ = RegisteredProof::StackedDRG2KiBSeal;
  RegisteredProof post_proof_ = RegisteredProof::StackedDRG2KiBPoSt;
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

TEST_F(SectorStorageTest, AddPiece) {
  fc::common::Blob<2032> some_bytes;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> dis(0, 255);
  for (size_t i = 0; i < some_bytes.size(); i++) {
    some_bytes[i] = dis(gen);
  }

  auto path_model = fs::canonical(base_path).append("%%%%%");
  Path piece_file_a_path = boost::filesystem::unique_path(path_model).string();
  boost::filesystem::ofstream piece_file_a(piece_file_a_path);

  UnpaddedPieceSize piece_commitment_a_size(127);
  for (size_t i = 0; i < piece_commitment_a_size; i++) {
    piece_file_a << some_bytes[i];
  }
  piece_file_a.close();

  PieceData file_a(piece_file_a_path);

  SectorId sector{
      .sector = 3,
      .miner = 1,
  };

  EXPECT_OUTCOME_TRUE(paths,
                      sector_storage_->acquireSector(
                          sector, SectorFileTypes::FTUnsealed, 0, true))

  EXPECT_OUTCOME_TRUE_1(
      sector_storage_->addPiece(sector, {}, piece_commitment_a_size, file_a));

  ASSERT_TRUE(fs::exists(paths.unsealed));
  ASSERT_TRUE(fs::file_size(paths.unsealed) == 128);

  Path piece_file_b_path = boost::filesystem::unique_path(path_model).string();
  boost::filesystem::ofstream piece_file_b(piece_file_b_path);

  UnpaddedPieceSize piece_commitment_b_size(1016);
  for (size_t i = 0; i < piece_commitment_b_size; i++) {
    piece_file_b << some_bytes[i];
  }
  piece_file_b.close();

  std::vector<UnpaddedPieceSize> exist_pieces = {piece_commitment_a_size};

  PieceData file_b(piece_file_b_path);

  EXPECT_OUTCOME_TRUE_1(sector_storage_->addPiece(
      sector, exist_pieces, piece_commitment_b_size, file_b));

  ASSERT_TRUE(fs::file_size(paths.unsealed) == 2048);
}

TEST_F(SectorStorageTest, Sealer_PreCommit_MatchSumError) {
  SectorId sector{
      .sector = 3,
      .miner = 1,
  };

  fc::proofs::SealRandomness ticket{{5, 4, 2}};

  EXPECT_OUTCOME_ERROR(SectorStorageError::DONOT_MATCH_SIZES,
                       sector_storage_->sealPreCommit1(sector, ticket, {}));
}

TEST_F(SectorStorageTest, Sealer) {
  EXPECT_OUTCOME_TRUE(sector_size,
                      fc::primitives::sector::getSectorSize(seal_proof_));

  fc::common::Blob<2032> some_bytes;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> dis(0, 255);
  for (size_t i = 0; i < some_bytes.size(); i++) {
    some_bytes[i] = dis(gen);
  }

  auto path_model = fs::canonical(base_path).append("%%%%%");
  Path piece_file_a_path = boost::filesystem::unique_path(path_model).string();
  boost::filesystem::ofstream piece_file_a(piece_file_a_path);

  UnpaddedPieceSize piece_commitment_a_size(2032);
  for (size_t i = 0; i < piece_commitment_a_size; i++) {
    piece_file_a << some_bytes[i];
  }
  piece_file_a.close();

  PieceData file_a(piece_file_a_path);

  SectorId sector{
      .sector = 3,
      .miner = 1,
  };

  EXPECT_OUTCOME_TRUE(
      paths,
      sector_storage_->acquireSector(sector, SectorFileTypes::FTCache, 0, true))

  EXPECT_OUTCOME_TRUE(
      a_info,
      sector_storage_->addPiece(sector, {}, piece_commitment_a_size, file_a));

  fc::proofs::SealRandomness ticket{{5, 4, 2}};

  std::vector<PieceInfo> pieces = {a_info};

  EXPECT_OUTCOME_TRUE(pc1o,
                      sector_storage_->sealPreCommit1(sector, ticket, pieces));

  EXPECT_OUTCOME_TRUE(cids, sector_storage_->sealPreCommit2(sector, pc1o));

  InteractiveRandomness seed{{7, 4, 2}};

  EXPECT_OUTCOME_TRUE(
      c1o, sector_storage_->sealCommit1(sector, ticket, seed, pieces, cids));

  EXPECT_OUTCOME_TRUE(proof, sector_storage_->sealCommit2(sector, c1o));

  EXPECT_OUTCOME_TRUE(valid,
                      proofs::verifySeal(SealVerifyInfo{
                          .sector = sector,
                          .info =
                              OnChainSealVerifyInfo{
                                  .sealed_cid = cids.sealed_cid,
                                  .registered_proof = seal_proof_,
                                  .proof = proof,
                                  .sector = sector.sector,
                              },
                          .randomness = ticket,
                          .interactive_randomness = seed,
                          .unsealed_cid = cids.unsealed_cid,
                      }));

  ASSERT_TRUE(valid);
}
