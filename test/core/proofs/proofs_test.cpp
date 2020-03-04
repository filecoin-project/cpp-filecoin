/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proofs.hpp"

#include <gtest/gtest.h>
#include <random>
#include "proofs/proof_param_provider.hpp"
#include "storage/filestore/impl/filesystem/filesystem_file.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using namespace fc::proofs;
using namespace boost::filesystem;
using fc::common::Blob;
using fc::crypto::randomness::Randomness;
using fc::storage::filestore::File;
using fc::storage::filestore::FileSystemFile;
using fc::storage::filestore::Path;

class ProofsTest : public test::BaseFS_Test {
 public:
  ProofsTest() : test::BaseFS_Test("fc_proofs_test") {
    auto res = fc::proofs::ProofParamProvider::readJson(
        "/var/tmp/filecoin-proof-parameters/parameters.json");
    if (!res.has_error()) {
      params = std::move(res.value());
    }
  }

 protected:
  std::vector<fc::proofs::ParamFile> params;
};

// TODO(artyom-yurin): [FIL-164]
/**
 * disabled because it takes too long
 * @given Data for PoSt generation
 * @when Generates and Verifies PoST
 * @then success
 */
TEST_F(ProofsTest, DISABLED_ValidPoSt) {
  uint64_t challenge_count = 2;
  Prover prover_id{{6, 7, 8}};
  Randomness randomness{{9, 9, 9}};
  Ticket ticket{{5, 4, 2}};
  Seed seed{{7, 4, 2}};
  fc::proofs::RegisteredProof sealProofType =
      fc::primitives::sector::RegisteredProof::StackedDRG1KiBSeal;
  fc::proofs::RegisteredProof postProofType =
      fc::primitives::sector::RegisteredProof::StackedDRG1KiBPoSt;
  SectorSize sector_size = 1024;
  SectorNumber sector_num = 42;
  EXPECT_OUTCOME_TRUE_1(
      fc::proofs::ProofParamProvider::getParams(params, sector_size));

  Path sector_cache_dir_path =
      boost::filesystem::unique_path(
          fs::canonical(base_path).append("%%%%%-sector-cache-dir"))
          .string();
  boost::filesystem::create_directory(sector_cache_dir_path);

  Path staged_sector_file =
      boost::filesystem::unique_path(
          fs::canonical(base_path).append("%%%%%-staged-sector-file"))
          .string();
  boost::filesystem::ofstream(staged_sector_file).close();

  Path sealed_sector_file =
      boost::filesystem::unique_path(
          fs::canonical(base_path).append("%%%%%-sealed-sector-file"))
          .string();
  boost::filesystem::ofstream(sealed_sector_file).close();

  Path unseal_output_file_a =
      boost::filesystem::unique_path(
          fs::canonical(base_path).append("%%%%%-unseal-output-file-a"))
          .string();
  boost::filesystem::ofstream(unseal_output_file_a).close();

  Path unseal_output_file_b =
      boost::filesystem::unique_path(
          fs::canonical(base_path).append("%%%%%-unseal-output-file-b"))
          .string();
  boost::filesystem::ofstream(unseal_output_file_b).close();

  Path unseal_output_file_c =
      boost::filesystem::unique_path(
          fs::canonical(base_path).append("%%%%%-unseal-output-file-c"))
          .string();
  boost::filesystem::ofstream(unseal_output_file_c).close();

  fc::common::Blob<1016> some_bytes;
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

  EXPECT_OUTCOME_TRUE(
      piece_cid_a,
      Proofs::generatePieceCIDFromFile(
          sealProofType, piece_file_a_path, UnpaddedPieceSize(127)));

  EXPECT_OUTCOME_TRUE(resA,
                      Proofs::writeWithoutAlignment(sealProofType,
                                                    piece_file_a_path,
                                                    piece_commitment_a_size,
                                                    staged_sector_file));
  ASSERT_EQ(resA.total_write_unpadded, piece_commitment_a_size);
  ASSERT_EQ(resA.piece_cid, piece_cid_a);

  Path piece_file_b_path = boost::filesystem::unique_path(path_model).string();
  boost::filesystem::ofstream piece_file_b(piece_file_b_path);

  UnpaddedPieceSize piece_commitment_b_size(508);
  for (size_t i = 0; i < piece_commitment_b_size; i++) {
    piece_file_b << some_bytes[i];
  }
  piece_file_b.close();

  EXPECT_OUTCOME_TRUE(
      piece_cid_b,
      Proofs::generatePieceCIDFromFile(
          sealProofType, piece_file_b_path, UnpaddedPieceSize(508)));

  std::vector<uint64_t> commitment = {piece_commitment_a_size};
  EXPECT_OUTCOME_TRUE(resB,
                      Proofs::writeWithAlignment(sealProofType,
                                                 piece_file_b_path,
                                                 piece_commitment_b_size,
                                                 staged_sector_file,
                                                 commitment));
  ASSERT_EQ(resB.left_alignment_unpadded,
            piece_commitment_b_size - piece_commitment_a_size);

  ASSERT_EQ(resB.left_alignment_unpadded, 381);
  ASSERT_EQ(resB.total_write_unpadded, 889);
  ASSERT_EQ(resB.piece_cid, piece_cid_b);

  std::vector<PieceInfo> public_pieces;
  public_pieces.emplace_back(piece_commitment_a_size.padded(), piece_cid_a);
  public_pieces.emplace_back(piece_commitment_b_size.padded(), piece_cid_b);

  EXPECT_OUTCOME_TRUE(
      preGeneratedUnsealedCID,
      Proofs::generateUnsealedCID(sealProofType, public_pieces));

  // pre-commit the sector
  EXPECT_OUTCOME_TRUE(sealPreCommitPhase1Output,
                      Proofs::sealPreCommitPhase1(sealProofType,
                                                  sector_cache_dir_path,
                                                  staged_sector_file,
                                                  sealed_sector_file,
                                                  sector_num,
                                                  prover_id,
                                                  ticket,
                                                  public_pieces));

  EXPECT_OUTCOME_TRUE(sealedAndUnsealedCID,
                      Proofs::sealPreCommitPhase2(sealPreCommitPhase1Output,
                                                  sector_cache_dir_path,
                                                  sealed_sector_file));

  ASSERT_EQ(sealedAndUnsealedCID.second, preGeneratedUnsealedCID);

  PrivateSectorInfo private_replica_info;
  private_replica_info.sector = sector_num;
  private_replica_info.sealed_cid = sealedAndUnsealedCID.first;
  private_replica_info.cache_dir_path = sector_cache_dir_path;
  private_replica_info.sealed_sector_path = sealed_sector_file;
  private_replica_info.post_proof_type = postProofType;

  std::vector<PrivateSectorInfo> private_replicas_info = {private_replica_info};
  auto private_info = Proofs::newSortedPrivateSectorInfo(private_replicas_info);

  PublicSectorInfo public_sector_info;
  public_sector_info.sector_num = sector_num;
  public_sector_info.post_proof_type = postProofType;
  public_sector_info.sealed_cid = sealedAndUnsealedCID.first;
  std::vector<PublicSectorInfo> public_sectors_info = {public_sector_info};
  auto public_info = Proofs::newSortedPublicSectorInfo(public_sectors_info);

  EXPECT_OUTCOME_TRUE(candidates_with_tickets,
                      Proofs::generateCandidates(
                          prover_id, randomness, challenge_count, private_info))

  std::vector<PoStCandidate> candidates = {};
  for (const auto &candidate_with_ticket : candidates_with_tickets) {
    candidates.push_back(candidate_with_ticket.candidate);
  }

  EXPECT_OUTCOME_TRUE(
      proof_a,
      Proofs::generatePoSt(prover_id, private_info, randomness, candidates))

  EXPECT_OUTCOME_TRUE(res,
                      Proofs::verifyPoSt(public_info,
                                         randomness,
                                         challenge_count,
                                         proof_a,
                                         candidates,
                                         prover_id));
  ASSERT_TRUE(res);
}
