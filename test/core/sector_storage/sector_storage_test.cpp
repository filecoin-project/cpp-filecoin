/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <random>

#include "proofs/proof_param_provider.hpp"
#include "sector_storage/impl/sector_storage_impl.hpp"
#include "sector_storage/sector_storage.hpp"
#include "sector_storage/sector_storage_error.hpp"
#include "storage/filestore/path.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using fc::primitives::sector::OnChainSealVerifyInfo;
using fc::primitives::sector::SealVerifyInfo;
using fc::primitives::sector_file::SectorFileType;
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

/**
 * @given sector
 * @when want to get path to sealed sector
 * @then sealed sector was obtained. cache and unsealed paths are empty
 */
TEST_F(SectorStorageTest, AcquireSector_Sealed) {
  std::string sealed_result =
      (base_path / fs::path("sealed") / fs::path("s-t01-3")).string();

  SectorId sector{
      .miner = 1,
      .sector = 3,
  };

  EXPECT_OUTCOME_TRUE(
      paths, sector_storage_->acquireSector(sector, SectorFileType::FTSealed))

  ASSERT_TRUE(paths.cache.empty());
  ASSERT_TRUE(paths.unsealed.empty());
  ASSERT_EQ(paths.sealed, sealed_result);
}

/**
 * @given sector
 * @when want to get path to sealed and cache sector
 * @then sealed sector was obtained. cache and unsealed paths are empty
 */
TEST_F(SectorStorageTest, AcquireSector_Complex) {
  std::string sealed_result =
      (base_path / fs::path("sealed") / fs::path("s-t01-3")).string();
  std::string cache_result =
      (base_path / fs::path("cache") / fs::path("s-t01-3")).string();

  SectorId sector{
      .miner = 1,
      .sector = 3,
  };

  EXPECT_OUTCOME_TRUE(
      paths,
      sector_storage_->acquireSector(
          sector,
          static_cast<SectorFileType>(SectorFileType::FTCache
                                      | SectorFileType::FTSealed)))

  ASSERT_EQ(paths.cache, cache_result);
  ASSERT_TRUE(paths.unsealed.empty());
  ASSERT_EQ(paths.sealed, sealed_result);
}

/**
 * @given sector
 * @when want to get path to unsealed sector
 * @then unsealed sector was obtained. cache and sealed paths are empty
 */
TEST_F(SectorStorageTest, AcquireSector_Unsealed) {
  std::string unsealed_result =
      (base_path / fs::path("unsealed") / fs::path("s-t01-3")).string();

  SectorId sector{
      .miner = 1,
      .sector = 3,
  };

  EXPECT_OUTCOME_TRUE(
      paths, sector_storage_->acquireSector(sector, SectorFileType::FTUnsealed))

  ASSERT_TRUE(paths.cache.empty());
  ASSERT_EQ(paths.unsealed, unsealed_result);
  ASSERT_TRUE(paths.sealed.empty());
}

/**
 * @given sector
 * @when want to get path to cache sector
 * @then all path were obtained
 */
TEST_F(SectorStorageTest, AcquireSector_Cache) {
  std::string cache_result =
      (base_path / fs::path("cache") / fs::path("s-t01-3")).string();

  SectorId sector{
      .miner = 1,
      .sector = 3,
  };

  EXPECT_OUTCOME_TRUE(
      paths, sector_storage_->acquireSector(sector, SectorFileType::FTCache))

  ASSERT_EQ(paths.cache, cache_result);
  ASSERT_TRUE(paths.unsealed.empty());
  ASSERT_TRUE(paths.sealed.empty());
}

/**
 * @given sector and pieces A and B
 * @when when add two pieces
 * @then pieces are succesfully added to sector
 */
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
      .miner = 1,
      .sector = 3,
  };

  EXPECT_OUTCOME_TRUE(
      paths, sector_storage_->acquireSector(sector, SectorFileType::FTUnsealed))

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

/**
 * @given sector
 * @when when try to preSealCommit with sum of sizes of pieces and not equal to
 * sector size
 * @then Error occures
 */
TEST_F(SectorStorageTest, Sealer_PreCommit_MatchSumError) {
  SectorId sector{
      .miner = 1,
      .sector = 3,
  };

  fc::proofs::SealRandomness ticket{{5, 4, 2}};

  EXPECT_OUTCOME_ERROR(SectorStorageError::DONOT_MATCH_SIZES,
                       sector_storage_->sealPreCommit1(sector, ticket, {}));
}

/**
 * @given sector and one piece with sector size
 * @when try to seal sector and verify it
 * @then success
 */
TEST_F(SectorStorageTest, Sealer) {
  EXPECT_OUTCOME_TRUE(
      params,
      fc::proofs::ProofParamProvider::readJson(
          "/var/tmp/filecoin-proof-parameters/parameters.json"));
  EXPECT_OUTCOME_TRUE(sector_size,
                      fc::primitives::sector::getSectorSize(seal_proof_));
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
  Path piece_file_a_path = boost::filesystem::unique_path(path_model).string();
  boost::filesystem::ofstream piece_file_a(piece_file_a_path);

  UnpaddedPieceSize piece_commitment_a_size(1016);
  for (size_t i = 0; i < piece_commitment_a_size; i++) {
    piece_file_a << some_bytes[i];
  }
  piece_file_a.close();

  Path piece_file_b_path = boost::filesystem::unique_path(path_model).string();
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

  EXPECT_OUTCOME_TRUE(
      a_info,
      sector_storage_->addPiece(sector, {}, piece_commitment_a_size, file_a));
  EXPECT_OUTCOME_TRUE(b_info,
                      sector_storage_->addPiece(sector,
                                                {{piece_commitment_a_size}},
                                                piece_commitment_b_size,
                                                file_b));

  fc::proofs::SealRandomness ticket{{5, 4, 2}};

  std::vector<PieceInfo> pieces = {a_info, b_info};

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
                                  .interactive_epoch = 42,
                                  .registered_proof = seal_proof_,
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

  EXPECT_OUTCOME_TRUE(
      paths,
      sector_storage_->acquireSector(sector, SectorFileType::FTUnsealed));
  boost::filesystem::remove(paths.unsealed);
  EXPECT_OUTCOME_TRUE(
      piece_a,
      sector_storage_->readPieceFromSealedSector(
          sector, 0, piece_commitment_a_size, ticket, cids.unsealed_cid));

  std::ifstream original_a_piece(piece_file_a_path, std::ifstream::binary);

  ASSERT_TRUE(original_a_piece.is_open());

  char original_ch;
  char piece_ch;

  for (uint64_t read_size = 0; read_size < piece_commitment_a_size;
       read_size++) {
    original_a_piece.read(&original_ch, 1);
    read(piece_a.getFd(), &piece_ch, 1);
    ASSERT_EQ(original_ch, piece_ch);
  }
}
