/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proofs.hpp"

#include <gtest/gtest.h>
#include <random>
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/filestore/impl/filesystem/filesystem_file.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using namespace fc::proofs;
using namespace boost::filesystem;
using fc::common::Blob;
using fc::crypto::randomness::Randomness;
using fc::primitives::piece::PaddedPieceSize;
using fc::primitives::sector::OnChainSealVerifyInfo;
using fc::primitives::sector::SealVerifyInfo;
using fc::primitives::sector::SectorId;
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
 * @given data of sector
 * @when want to seal data and proof post
 * @then success
 */
TEST_F(ProofsTest, Lifecycle) {
  uint64_t challenge_count = 2;
  ActorId miner_id = 42;
  Randomness randomness{{9, 9, 9}};
  fc::proofs::RegisteredProof seal_proof_type =
      fc::primitives::sector::RegisteredProof::StackedDRG2KiBSeal;
  fc::proofs::RegisteredProof post_proof_type =
      fc::primitives::sector::RegisteredProof::StackedDRG2KiBPoSt;
  SectorNumber sector_num = 42;

  Ticket ticket{{5, 4, 2}};

  Seed seed{{7, 4, 2}};

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

  EXPECT_OUTCOME_TRUE(
      piece_cid_a,
      Proofs::generatePieceCIDFromFile(
          seal_proof_type, piece_file_a_path, UnpaddedPieceSize(127)));

  EXPECT_OUTCOME_TRUE(res_a,
                      Proofs::writeWithoutAlignment(seal_proof_type,
                                                    piece_file_a_path,
                                                    piece_commitment_a_size,
                                                    staged_sector_file));

  ASSERT_EQ(res_a.total_write_unpadded, 127);
  ASSERT_EQ(res_a.piece_cid, piece_cid_a);

  Path piece_file_b_path = boost::filesystem::unique_path(path_model).string();
  boost::filesystem::ofstream piece_file_b(piece_file_b_path);

  UnpaddedPieceSize piece_commitment_b_size(1016);
  for (size_t i = 0; i < piece_commitment_b_size; i++) {
    piece_file_b << some_bytes[i];
  }
  piece_file_b.close();

  EXPECT_OUTCOME_TRUE(
      piece_cid_b,
      Proofs::generatePieceCIDFromFile(
          seal_proof_type, piece_file_b_path, UnpaddedPieceSize(1016)));

  std::vector<UnpaddedPieceSize> exist_pieces = {piece_commitment_a_size};
  EXPECT_OUTCOME_TRUE(res_b,
                      Proofs::writeWithAlignment(seal_proof_type,
                                                 piece_file_b_path,
                                                 piece_commitment_b_size,
                                                 staged_sector_file,
                                                 exist_pieces));

  ASSERT_EQ(res_b.left_alignment_unpadded, 889);

  ASSERT_EQ(res_b.total_write_unpadded, 1905);

  ASSERT_EQ(res_b.piece_cid, piece_cid_b);

  std::vector<PieceInfo> public_pieces = {
      PieceInfo(piece_commitment_a_size.padded(), piece_cid_a),
      PieceInfo(piece_commitment_b_size.padded(), piece_cid_b),
  };

  EXPECT_OUTCOME_TRUE(
      pregenerated_unsealed_cid,
      Proofs::generateUnsealedCID(seal_proof_type, public_pieces));

  EXPECT_OUTCOME_TRUE(seal_precommit_phase1_output,
                      Proofs::sealPreCommitPhase1(seal_proof_type,
                                                  sector_cache_dir_path,
                                                  staged_sector_file,
                                                  sealed_sector_file,
                                                  sector_num,
                                                  miner_id,
                                                  ticket,
                                                  public_pieces));

  EXPECT_OUTCOME_TRUE(sealed_and_unsealed_cid,
                      Proofs::sealPreCommitPhase2(seal_precommit_phase1_output,
                                                  sector_cache_dir_path,
                                                  sealed_sector_file));

  ASSERT_EQ(sealed_and_unsealed_cid.unsealed_cid, pregenerated_unsealed_cid);

  // commit the sector
  EXPECT_OUTCOME_TRUE(
      seal_commit_phase1_output,
      Proofs::sealCommitPhase1(seal_proof_type,
                               sealed_and_unsealed_cid.sealed_cid,
                               sealed_and_unsealed_cid.unsealed_cid,
                               sector_cache_dir_path,
                               sealed_sector_file,
                               sector_num,
                               miner_id,
                               ticket,
                               seed,
                               public_pieces))
  EXPECT_OUTCOME_TRUE(seal_proof,
                      Proofs::sealCommitPhase2(
                          seal_commit_phase1_output, sector_num, miner_id));

  EXPECT_OUTCOME_TRUE(
      isValid,
      Proofs::verifySeal(SealVerifyInfo{
          .sector =
              SectorId{
                  .miner = miner_id,
                  .sector = sector_num,
              },
          .info =
              OnChainSealVerifyInfo{
                  .sealed_cid = sealed_and_unsealed_cid.sealed_cid,
                  .interactive_epoch = 42,
                  .registered_proof = seal_proof_type,
                  .proof = seal_proof,
                  .deals = {},
                  .sector = sector_num,
                  .seal_rand_epoch = 42,
              },
          .randomness = ticket,
          .interactive_randomness = seed,
          .unsealed_cid = sealed_and_unsealed_cid.unsealed_cid,
      }));

  ASSERT_TRUE(isValid);

  EXPECT_OUTCOME_TRUE_1(Proofs::unseal(seal_proof_type,
                                       sector_cache_dir_path,
                                       sealed_sector_file,
                                       unseal_output_file_a,
                                       sector_num,
                                       miner_id,
                                       ticket,
                                       sealed_and_unsealed_cid.unsealed_cid));

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
  ASSERT_EQ(gsl::make_span(file_a_bytes.data() + 1016, 1016),
            gsl::make_span(some_bytes.data(), 1016));

  EXPECT_OUTCOME_TRUE_1(
      Proofs::unsealRange(seal_proof_type,
                          sector_cache_dir_path,
                          sealed_sector_file,
                          unseal_output_file_b,
                          sector_num,
                          miner_id,
                          ticket,
                          sealed_and_unsealed_cid.unsealed_cid,
                          0,
                          127));

  std::vector<uint8_t> file_b_bytes = read_file(unseal_output_file_b);

  ASSERT_EQ(gsl::make_span(file_b_bytes),
            gsl::make_span(some_bytes.data(), 127));

  EXPECT_OUTCOME_TRUE_1(
      Proofs::unsealRange(seal_proof_type,
                          sector_cache_dir_path,
                          sealed_sector_file,
                          unseal_output_file_c,
                          sector_num,
                          miner_id,
                          ticket,
                          sealed_and_unsealed_cid.unsealed_cid,
                          1016,
                          1016));

  std::vector<uint8_t> file_c_bytes = read_file(unseal_output_file_c);

  ASSERT_EQ(gsl::make_span(file_c_bytes),
            gsl::make_span(some_bytes.data(), 1016));

  std::vector<PrivateSectorInfo> private_replicas_info = {PrivateSectorInfo{
      .info =
          SectorInfo{
              .sector = sector_num,
              .sealed_cid = sealed_and_unsealed_cid.sealed_cid,
          },
      .cache_dir_path = sector_cache_dir_path,
      .post_proof_type = post_proof_type,
      .sealed_sector_path = sealed_sector_file,
  }};
  auto private_info = Proofs::newSortedPrivateSectorInfo(private_replicas_info);

  std::vector<PublicSectorInfo> public_sectors_info = {PublicSectorInfo{
      .post_proof_type = post_proof_type,
      .sealed_cid = sealed_and_unsealed_cid.sealed_cid,
      .sector_num = sector_num,
  }};
  auto public_info = Proofs::newSortedPublicSectorInfo(public_sectors_info);

  std::vector<SectorInfo> elignable_sectors = {SectorInfo{
      .registered_proof = seal_proof_type,
      .sector = sector_num,
      .sealed_cid = sealed_and_unsealed_cid.sealed_cid,
  }};

  EXPECT_OUTCOME_TRUE(candidates_with_tickets,
                      Proofs::generateCandidates(
                          miner_id, randomness, challenge_count, private_info))

  std::vector<PoStCandidate> candidates = {};
  for (const auto &candidate_with_ticket : candidates_with_tickets) {
    candidates.push_back(candidate_with_ticket.candidate);
  }

  EXPECT_OUTCOME_TRUE(final_ticket,
                      Proofs::finalizeTicket(candidates[0].partial_ticket));
  ASSERT_EQ(final_ticket, candidates_with_tickets[0].ticket);

  EXPECT_OUTCOME_TRUE(
      proofs,
      Proofs::generatePoSt(miner_id, private_info, randomness, candidates))

  EXPECT_OUTCOME_TRUE(res,
                      Proofs::verifyPoSt(PoStVerifyInfo{
                          .randomness = randomness,
                          .candidates = candidates,
                          .proofs = proofs,
                          .eligible_sectors = elignable_sectors,
                          .prover = miner_id,
                          .challenge_count = challenge_count,
                      }));
  ASSERT_TRUE(res);
}
