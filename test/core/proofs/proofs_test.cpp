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

/**
 * @given Data for PoSt generation
 * @when Generates and Verifies PoST
 * @then success
 */
TEST_F(ProofsTest, DISABLED_ValidPoSt) {
  uint64_t challenge_count = 2;
  uint8_t porep_proof_partitions = 10;
  Blob<32> prover_id{{6, 7, 8}};
  Randomness randomness{{9, 9, 9}};
  Blob<32> ticket{{5, 4, 2}};
  uint64_t sector_size = 1024;
  uint64_t sector_id = 42;
  EXPECT_OUTCOME_TRUE_1(
      fc::proofs::ProofParamProvider::getParams(params, sector_size));

  Path sector_cache_dir_path =
      unique_path(fs::canonical(base_path).append("%%%%%-sector-cache-dir"))
          .string();
  create_directory(sector_cache_dir_path);

  Path staged_sector_file =
      unique_path(fs::canonical(base_path).append("%%%%%-staged-sector-file"))
          .string();
  ofstream(staged_sector_file).close();

  Path sealed_sector_file =
      unique_path(fs::canonical(base_path).append("%%%%%-sealed-sector-file"))
          .string();
  ofstream(sealed_sector_file).close();

  Blob<1016> some_bytes;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> dis(0, 255);
  for (size_t i = 0; i < some_bytes.size(); i++) {
    some_bytes[i] = dis(gen);
  }

  auto path_model = fs::canonical(base_path).append("%%%%%");
  Path piece_file_a_path = unique_path(path_model).string();
  ofstream piece_file_a(piece_file_a_path);

  int piece_commitment_a_size = 127;
  for (int i = 0; i < piece_commitment_a_size; i++) {
    piece_file_a << some_bytes[i];
  }
  piece_file_a.close();

  Path piece_file_b_path = unique_path(path_model).string();
  ofstream piece_file_b(piece_file_b_path);

  int piece_commitment_b_size = 508;
  for (int i = 0; i < piece_commitment_b_size; i++) {
    piece_file_b << some_bytes[i];
  }
  piece_file_b.close();

  std::vector<PublicPieceInfo> public_pieces;

  PublicPieceInfo pA;
  pA.size = piece_commitment_a_size;
  EXPECT_OUTCOME_TRUE(piece_commitment_a,
                      generatePieceCommitmentFromFile(piece_file_a_path,
                                                      piece_commitment_a_size));
  pA.comm_p = piece_commitment_a;

  EXPECT_OUTCOME_TRUE(
      resA,
      writeWithoutAlignment(
          piece_file_a_path, piece_commitment_a_size, staged_sector_file));
  ASSERT_EQ(resA.total_write_unpadded, piece_commitment_a_size);
  ASSERT_EQ(resA.comm_p, pA.comm_p);

  std::vector<uint64_t> commitment = {127};
  EXPECT_OUTCOME_TRUE(resB,
                      writeWithAlignment(piece_file_b_path,
                                         piece_commitment_b_size,
                                         staged_sector_file,
                                         gsl::span<uint64_t>(commitment)));
  ASSERT_EQ(resB.left_alignment_unpadded,
            piece_commitment_b_size - piece_commitment_a_size);

  PublicPieceInfo pB;
  pB.size = piece_commitment_b_size;
  EXPECT_OUTCOME_TRUE(piece_commitment_b,
                      generatePieceCommitmentFromFile(piece_file_b_path,
                                                      piece_commitment_b_size));
  pB.comm_p = piece_commitment_b;
  ASSERT_EQ(resB.left_alignment_unpadded, 381);
  ASSERT_EQ(resB.total_write_unpadded, 889);
  ASSERT_EQ(resB.comm_p, pB.comm_p);

  public_pieces.push_back(pA);
  public_pieces.push_back(pB);

  EXPECT_OUTCOME_TRUE(comm_d,
                      generateDataCommitment(sector_size, public_pieces));

  // pre-commit the sector
  EXPECT_OUTCOME_TRUE(output,
                      sealPreCommit(sector_size,
                                    porep_proof_partitions,
                                    sector_cache_dir_path,
                                    staged_sector_file,
                                    sealed_sector_file,
                                    sector_id,
                                    prover_id,
                                    ticket,
                                    public_pieces));

  ASSERT_EQ(comm_d, output.comm_d);

  PrivateReplicaInfo private_replica_info;
  private_replica_info.sector_id = sector_id;
  private_replica_info.comm_r = output.comm_r;
  private_replica_info.cache_dir_path = sector_cache_dir_path;
  private_replica_info.sealed_sector_path = sealed_sector_file;
  std::vector<PrivateReplicaInfo> private_replicas_info = {
      private_replica_info};
  auto private_info = newSortedPrivateReplicaInfo(private_replicas_info);

  PublicSectorInfo public_sector_info;
  public_sector_info.sector_id = sector_id;
  public_sector_info.comm_r = output.comm_r;
  std::vector<PublicSectorInfo> public_sectors_info = {public_sector_info};
  auto public_info = newSortedPublicSectorInfo(public_sectors_info);

  EXPECT_OUTCOME_TRUE(
      candidates,
      generateCandidates(
          sector_size, prover_id, randomness, challenge_count, private_info));

  EXPECT_OUTCOME_TRUE(
      proof_a,
      generatePoSt(
          sector_size, prover_id, private_info, randomness, candidates))

  EXPECT_OUTCOME_TRUE(res,
                      verifyPoSt(sector_size,
                                 public_info,
                                 randomness,
                                 challenge_count,
                                 proof_a,
                                 candidates,
                                 prover_id));
  ASSERT_TRUE(res);
}
