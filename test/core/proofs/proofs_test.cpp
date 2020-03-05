/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proofs.hpp"

#include <gtest/gtest.h>
#include <random>
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
  ProofsTest() : test::BaseFS_Test("fc_proofs_test") {}
};

// TODO(artyom-yurin): [FIL-164]
/**
 * disabled because it takes too long
 * @given Data for PoSt generation
 * @when Generates and Verifies PoST
 * @then success
 */
TEST_F(ProofsTest, Lifecycle) {
  uint64_t challenge_count = 2;
  Prover prover_id{{6, 7, 8}};
  Randomness randomness{{9, 9, 9}};
  Ticket ticket{{5, 4, 2}};
  Seed seed{{7, 4, 2}};
  fc::proofs::RegisteredProof seal_proof_type =
      fc::primitives::sector::RegisteredProof::StackedDRG1KiBSeal;
  fc::proofs::RegisteredProof post_proof_type =
      fc::primitives::sector::RegisteredProof::StackedDRG1KiBPoSt;
  SectorNumber sector_num = 42;

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
          seal_proof_type, piece_file_a_path, UnpaddedPieceSize(127)));

  EXPECT_OUTCOME_TRUE(resA,
                      Proofs::writeWithoutAlignment(seal_proof_type,
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
          seal_proof_type, piece_file_b_path, UnpaddedPieceSize(508)));

  std::vector<uint64_t> commitment = {piece_commitment_a_size};
  EXPECT_OUTCOME_TRUE(resB,
                      Proofs::writeWithAlignment(seal_proof_type,
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
      Proofs::generateUnsealedCID(seal_proof_type, public_pieces));

  // pre-commit the sector
  EXPECT_OUTCOME_TRUE(sealPreCommitPhase1Output,
                      Proofs::sealPreCommitPhase1(seal_proof_type,
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

  // commit the sector
  EXPECT_OUTCOME_TRUE(seal_commit_phase1_output,
                      Proofs::sealCommitPhase1(seal_proof_type,
                                               sealedAndUnsealedCID.first,
                                               sealedAndUnsealedCID.second,
                                               sector_cache_dir_path,
                                               sector_num,
                                               prover_id,
                                               ticket,
                                               seed,
                                               public_pieces))
  EXPECT_OUTCOME_TRUE(seal_proof,
                      Proofs::sealCommitPhase2(
                          seal_commit_phase1_output, sector_num, prover_id));

  EXPECT_OUTCOME_TRUE(isValid,
                      Proofs::verifySeal(seal_proof_type,
                                         sealedAndUnsealedCID.first,
                                         sealedAndUnsealedCID.second,
                                         prover_id,
                                         ticket,
                                         seed,
                                         sector_num,
                                         seal_proof));
  ASSERT_TRUE(isValid);

  EXPECT_OUTCOME_TRUE_1(Proofs::unseal(seal_proof_type,
                                       sector_cache_dir_path,
                                       sealed_sector_file,
                                       unseal_output_file_a,
                                       sector_num,
                                       prover_id,
                                       ticket,
                                       sealedAndUnsealedCID.second));

  auto read_file = [](const std::string &path) -> std::vector<uint8_t> {
    std::ifstream file(path, std::ios::binary);

    std::vector<uint8_t> bytes = {};

    if (!file.is_open()) return bytes;

    char ch;
    while (file.get(ch)) {
      bytes.push_back(ch);
    }

    file.close();
    return bytes;
  };

  std::vector<uint8_t> file_a_bytes = read_file(unseal_output_file_a);

  ASSERT_EQ(gsl::make_span(file_a_bytes.data(), 127),
            gsl::make_span(some_bytes.data(), 127));
  ASSERT_EQ(gsl::make_span(file_a_bytes.data() + 508, 508),
            gsl::make_span(some_bytes.data(), 508));

  EXPECT_OUTCOME_TRUE_1(Proofs::unsealRange(seal_proof_type,
                                            sector_cache_dir_path,
                                            sealed_sector_file,
                                            unseal_output_file_b,
                                            sector_num,
                                            prover_id,
                                            ticket,
                                            sealedAndUnsealedCID.second,
                                            0,
                                            127));

  std::vector<uint8_t> file_b_bytes = read_file(unseal_output_file_b);

  ASSERT_EQ(gsl::make_span(file_b_bytes),
            gsl::make_span(some_bytes.data(), 127));

  EXPECT_OUTCOME_TRUE_1(Proofs::unsealRange(seal_proof_type,
                                            sector_cache_dir_path,
                                            sealed_sector_file,
                                            unseal_output_file_c,
                                            sector_num,
                                            prover_id,
                                            ticket,
                                            sealedAndUnsealedCID.second,
                                            508,
                                            508));

  std::vector<uint8_t> file_c_bytes = read_file(unseal_output_file_c);

  ASSERT_EQ(gsl::make_span(file_c_bytes),
            gsl::make_span(some_bytes.data(), 508));

  std::vector<PrivateSectorInfo> private_replicas_info = {PrivateSectorInfo{
      .info =
          SectorInfo{
              .sector = sector_num,
              .sealed_cid = sealedAndUnsealedCID.first,
          },
      .cache_dir_path = sector_cache_dir_path,
      .sealed_sector_path = sealed_sector_file,
      .post_proof_type = post_proof_type,
  }};
  auto private_info = Proofs::newSortedPrivateSectorInfo(private_replicas_info);

  std::vector<PublicSectorInfo> public_sectors_info = {PublicSectorInfo{
      .sector_num = sector_num,
      .post_proof_type = post_proof_type,
      .sealed_cid = sealedAndUnsealedCID.first,
  }};
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
