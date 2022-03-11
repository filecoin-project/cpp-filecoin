/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/impl/proof_engine_impl.hpp"

#include <random>

#include <gtest/gtest.h>

#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/piece/piece_data.hpp"
#include "primitives/sector/sector.hpp"
#include "proofs/proof_param_provider.hpp"
#include "sector_storage/zerocomm/zerocomm.hpp"
#include "storage/filestore/impl/filesystem/filesystem_file.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::proofs {
  using common::Blob;
  using crypto::randomness::Randomness;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::piece::PaddedPieceSize;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::SealVerifyInfo;
  using primitives::sector::SectorId;
  using primitives::sector::SectorInfo;
  using primitives::sector::Ticket;
  using primitives::sector::WinningPoStVerifyInfo;
  using storage::filestore::File;
  using storage::filestore::FileSystemFile;
  using storage::filestore::Path;

  inline void touch(const std::string &path, uint64_t size) {
    std::ofstream{path};
    fs::resize_file(path, size);
  }

  class ProofsTest : public test::BaseFS_Test {
   public:
    ProofsTest() : test::BaseFS_Test("fc_proofs_test") {
      proofs_ = std::make_shared<ProofEngineImpl>();
    }

   protected:
    std::shared_ptr<ProofEngine> proofs_;
  };

  /**
   * @given data of sector
   * @when want to seal data and proof post
   * @then success
   */
  TEST_F(ProofsTest, Lifecycle) {
    ActorId miner_id = 42;
    Randomness randomness{{9, 9, 9}};
    RegisteredSealProof seal_proof_type =
        primitives::sector::RegisteredSealProof::kStackedDrg2KiBV1;
    RegisteredPoStProof winning_post_proof_type =
        primitives::sector::RegisteredPoStProof::kStackedDRG2KiBWinningPoSt;
    SectorNumber sector_num = 42;
    EXPECT_OUTCOME_TRUE(sector_size,
                        primitives::sector::getSectorSize(seal_proof_type));
    EXPECT_OUTCOME_TRUE_1(getParams(
        "/var/tmp/filecoin-proof-parameters/parameters.json", sector_size));

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

    common::Blob<2032> some_bytes;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for (size_t i = 0; i < 2032; i++) {
      some_bytes[i] = dis(gen);
    }

    auto path_model = fs::canonical(base_path).append("%%%%%");
    Path piece_file_a_path =
        boost::filesystem::unique_path(path_model).string();
    boost::filesystem::ofstream piece_file_a(piece_file_a_path);

    UnpaddedPieceSize piece_commitment_a_size(1016);
    for (size_t i = 0; i < piece_commitment_a_size; i++) {
      piece_file_a << some_bytes[i];
    }
    piece_file_a.close();

    PieceData file_a(piece_file_a_path);

    EXPECT_OUTCOME_TRUE(
        piece_cid_a,
        proofs_->generatePieceCIDFromFile(
            seal_proof_type, piece_file_a_path, UnpaddedPieceSize(1016)));

    EXPECT_OUTCOME_TRUE(res_a,
                        proofs_->writeWithoutAlignment(seal_proof_type,
                                                       file_a,
                                                       piece_commitment_a_size,
                                                       staged_sector_file));

    ASSERT_EQ(res_a.total_write_unpadded, 1016);
    ASSERT_EQ(res_a.piece_cid, piece_cid_a);

    Path piece_file_b_path =
        boost::filesystem::unique_path(path_model).string();
    boost::filesystem::ofstream piece_file_b(piece_file_b_path);

    UnpaddedPieceSize piece_commitment_b_size(1016);
    for (size_t i = piece_commitment_a_size;
         i < piece_commitment_a_size + piece_commitment_b_size;
         i++) {
      piece_file_b << some_bytes[i];
    }
    piece_file_b.close();

    EXPECT_OUTCOME_TRUE(
        piece_cid_b,
        proofs_->generatePieceCIDFromFile(
            seal_proof_type, piece_file_b_path, UnpaddedPieceSize(1016)));

    std::vector<UnpaddedPieceSize> exist_pieces = {piece_commitment_a_size};

    PieceData file_b(piece_file_b_path);
    EXPECT_OUTCOME_TRUE(res_b,
                        proofs_->writeWithAlignment(seal_proof_type,
                                                    file_b,
                                                    piece_commitment_b_size,
                                                    staged_sector_file,
                                                    exist_pieces));

    ASSERT_EQ(res_b.left_alignment_unpadded, 0);
    ASSERT_EQ(res_b.total_write_unpadded, 1016);
    ASSERT_EQ(res_b.piece_cid, piece_cid_b);

    std::vector<PieceInfo> public_pieces = {
        PieceInfo{.size = piece_commitment_a_size.padded(), .cid = piece_cid_a},
        PieceInfo{.size = piece_commitment_b_size.padded(), .cid = piece_cid_b},
    };

    EXPECT_OUTCOME_TRUE(
        pregenerated_unsealed_cid,
        proofs_->generateUnsealedCID(seal_proof_type, public_pieces, false));

    EXPECT_OUTCOME_TRUE(seal_precommit_phase1_output,
                        proofs_->sealPreCommitPhase1(seal_proof_type,
                                                     sector_cache_dir_path,
                                                     staged_sector_file,
                                                     sealed_sector_file,
                                                     sector_num,
                                                     miner_id,
                                                     ticket,
                                                     public_pieces));

    EXPECT_OUTCOME_TRUE(
        sealed_and_unsealed_cid,
        proofs_->sealPreCommitPhase2(seal_precommit_phase1_output,
                                     sector_cache_dir_path,
                                     sealed_sector_file));

    ASSERT_EQ(sealed_and_unsealed_cid.unsealed_cid, pregenerated_unsealed_cid);

    // commit the sector
    EXPECT_OUTCOME_TRUE(
        seal_commit_phase1_output,
        proofs_->sealCommitPhase1(seal_proof_type,
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
                        proofs_->sealCommitPhase2(
                            seal_commit_phase1_output, sector_num, miner_id));

    EXPECT_OUTCOME_TRUE(
        isValid,
        proofs_->verifySeal(SealVerifyInfo{
            .seal_proof = seal_proof_type,
            .sector =
                SectorId{
                    .miner = miner_id,
                    .sector = sector_num,
                },
            .deals = {},
            .randomness = ticket,
            .interactive_randomness = seed,
            .proof = seal_proof,
            .sealed_cid = sealed_and_unsealed_cid.sealed_cid,
            .unsealed_cid = sealed_and_unsealed_cid.unsealed_cid,
        }));

    ASSERT_TRUE(isValid);

    AggregateSealVerifyProofAndInfos aggregate{
        miner_id,
        seal_proof_type,
        primitives::sector::RegisteredAggregationProof::SnarkPackV1,
        {},
        {primitives::sector::AggregateSealVerifyInfo{
            sector_num,
            ticket,
            seed,
            sealed_and_unsealed_cid.sealed_cid,
            sealed_and_unsealed_cid.unsealed_cid,
        }},
    };
    EXPECT_OUTCOME_TRUE_1(
        proofs_->aggregateSealProofs(aggregate, {seal_proof}));
    EXPECT_OUTCOME_EQ(proofs_->verifyAggregateSeals(aggregate), true);

    EXPECT_OUTCOME_TRUE_1(
        proofs_->unseal(seal_proof_type,
                        sector_cache_dir_path,
                        sealed_sector_file,
                        unseal_output_file_a,
                        sector_num,
                        miner_id,
                        ticket,
                        sealed_and_unsealed_cid.unsealed_cid));

    auto file_a_bytes = readFile(unseal_output_file_a);

    ASSERT_EQ(gsl::make_span(file_a_bytes.data(), 1016),
              gsl::make_span(some_bytes.data(), 1016));
    ASSERT_EQ(gsl::make_span(file_a_bytes.data() + 1016, 1016),
              gsl::make_span(some_bytes.data() + 1016, 1016));

    EXPECT_OUTCOME_TRUE_1(
        proofs_->unsealRange(seal_proof_type,
                             sector_cache_dir_path,
                             sealed_sector_file,
                             unseal_output_file_b,
                             sector_num,
                             miner_id,
                             ticket,
                             sealed_and_unsealed_cid.unsealed_cid,
                             0,
                             1016));

    auto file_b_bytes = readFile(unseal_output_file_b);

    ASSERT_EQ(gsl::make_span(file_b_bytes),
              gsl::make_span(some_bytes.data(), 1016));

    EXPECT_OUTCOME_TRUE_1(
        proofs_->unsealRange(seal_proof_type,
                             sector_cache_dir_path,
                             sealed_sector_file,
                             unseal_output_file_c,
                             sector_num,
                             miner_id,
                             ticket,
                             sealed_and_unsealed_cid.unsealed_cid,
                             1016,
                             1016));

    auto file_c_bytes = readFile(unseal_output_file_c);

    ASSERT_EQ(gsl::make_span(file_c_bytes),
              gsl::make_span(some_bytes.data() + 1016, 1016));

    std::vector<PrivateSectorInfo> private_replicas_info = {};
    private_replicas_info.push_back(PrivateSectorInfo{
        .info =
            SectorInfo{
                .registered_proof = seal_proof_type,
                .sector = sector_num,
                .sealed_cid = sealed_and_unsealed_cid.sealed_cid,
            },
        .cache_dir_path = sector_cache_dir_path,
        .post_proof_type = winning_post_proof_type,
        .sealed_sector_path = sealed_sector_file,
    });
    auto private_info =
        proofs::newSortedPrivateSectorInfo(private_replicas_info);

    std::vector<SectorInfo> proving_set = {SectorInfo{
        .registered_proof = seal_proof_type,
        .sector = sector_num,
        .sealed_cid = sealed_and_unsealed_cid.sealed_cid,
    }};

    EXPECT_OUTCOME_TRUE(
        indices_in_proving_set,
        proofs_->generateWinningPoStSectorChallenge(
            winning_post_proof_type, miner_id, randomness, proving_set.size()));

    std::vector<SectorInfo> challenged_sectors;

    for (auto index : indices_in_proving_set) {
      challenged_sectors.push_back(proving_set.at(index));
    }

    EXPECT_OUTCOME_TRUE(
        proofs,
        proofs_->generateWinningPoSt(miner_id, private_info, randomness));

    EXPECT_OUTCOME_TRUE(res,
                        proofs_->verifyWinningPoSt(WinningPoStVerifyInfo{
                            .randomness = randomness,
                            .proofs = proofs,
                            .challenged_sectors = challenged_sectors,
                            .prover = miner_id,
                        }));
    ASSERT_TRUE(res);
  }

  /**
   * @given 5 pieces
   * @when write all in one file and then read it
   * @then pieces are identical
   */
  TEST_F(ProofsTest, WriteAndReadPiecesFile) {
    RegisteredSealProof seal_proof_type =
        primitives::sector::RegisteredSealProof::kStackedDrg2KiBV1;

    common::Blob<2032> some_bytes;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for (size_t i = 0; i < 2032; i++) {
      some_bytes[i] = dis(gen);
    }

    size_t start = 0;

    auto path_model = fs::canonical(base_path).append("%%%%%");
    Path unseal_path = boost::filesystem::unique_path(path_model).string();

    Path piece_file_a_path =
        boost::filesystem::unique_path(path_model).string();
    boost::filesystem::ofstream piece_file_a(piece_file_a_path);

    UnpaddedPieceSize piece_commitment_a_size(254);
    for (size_t i = start; i < start + piece_commitment_a_size; i++) {
      piece_file_a << some_bytes[i];
    }
    piece_file_a.close();

    start += piece_commitment_a_size;
    PieceData file_a(piece_file_a_path);

    EXPECT_OUTCOME_TRUE(
        piece_cid_a,
        proofs_->generatePieceCIDFromFile(
            seal_proof_type, piece_file_a_path, UnpaddedPieceSize(254)));

    EXPECT_OUTCOME_TRUE(
        res_a,
        proofs_->writeWithoutAlignment(
            seal_proof_type, file_a, piece_commitment_a_size, unseal_path));

    ASSERT_EQ(res_a.total_write_unpadded, 254);
    ASSERT_EQ(res_a.piece_cid, piece_cid_a);

    std::vector<Path> paths = {piece_file_a_path};
    std::vector<UnpaddedPieceSize> exist_pieces = {piece_commitment_a_size};

    Path piece_file_b_path =
        boost::filesystem::unique_path(path_model).string();
    boost::filesystem::ofstream piece_file_b(piece_file_b_path);

    UnpaddedPieceSize piece_commitment_b_size(1016);
    for (size_t i = start; i < start + piece_commitment_b_size; i++) {
      piece_file_b << some_bytes[i];
    }
    piece_file_b.close();

    EXPECT_OUTCOME_TRUE(
        piece_cid_b,
        proofs_->generatePieceCIDFromFile(
            seal_proof_type, piece_file_b_path, UnpaddedPieceSize(1016)));

    PieceData file_b(piece_file_b_path);
    EXPECT_OUTCOME_TRUE(res_b,
                        proofs_->writeWithAlignment(seal_proof_type,
                                                    file_b,
                                                    piece_commitment_b_size,
                                                    unseal_path,
                                                    exist_pieces));

    ASSERT_EQ(res_b.total_write_unpadded, 1016);
    ASSERT_EQ(res_b.piece_cid, piece_cid_b);

    start += piece_commitment_b_size;
    paths.push_back(piece_file_b_path);
    exist_pieces.push_back(piece_commitment_b_size);

    Path piece_file_c_path =
        boost::filesystem::unique_path(path_model).string();
    boost::filesystem::ofstream piece_file_c(piece_file_c_path);

    UnpaddedPieceSize piece_commitment_c_size(254);
    for (size_t i = start; i < start + piece_commitment_c_size; i++) {
      piece_file_c << some_bytes[i];
    }
    piece_file_c.close();

    EXPECT_OUTCOME_TRUE(
        piece_cid_c,
        proofs_->generatePieceCIDFromFile(
            seal_proof_type, piece_file_c_path, UnpaddedPieceSize(254)));

    PieceData file_c(piece_file_c_path);
    EXPECT_OUTCOME_TRUE(res_c,
                        proofs_->writeWithAlignment(seal_proof_type,
                                                    file_c,
                                                    piece_commitment_c_size,
                                                    unseal_path,
                                                    exist_pieces));

    ASSERT_EQ(res_c.total_write_unpadded, 254);
    ASSERT_EQ(res_c.piece_cid, piece_cid_c);
    start += piece_commitment_c_size;
    paths.push_back(piece_file_c_path);
    exist_pieces.push_back(piece_commitment_c_size);

    Path piece_file_d_path =
        boost::filesystem::unique_path(path_model).string();
    boost::filesystem::ofstream piece_file_d(piece_file_d_path);

    UnpaddedPieceSize piece_commitment_d_size(254);
    for (size_t i = start; i < start + piece_commitment_d_size; i++) {
      piece_file_d << some_bytes[i];
    }
    piece_file_d.close();

    EXPECT_OUTCOME_TRUE(
        piece_cid_d,
        proofs_->generatePieceCIDFromFile(
            seal_proof_type, piece_file_d_path, UnpaddedPieceSize(254)));

    PieceData file_d(piece_file_d_path);
    EXPECT_OUTCOME_TRUE(res_d,
                        proofs_->writeWithAlignment(seal_proof_type,
                                                    file_d,
                                                    piece_commitment_d_size,
                                                    unseal_path,
                                                    exist_pieces));

    ASSERT_EQ(res_d.total_write_unpadded, 254);
    ASSERT_EQ(res_d.piece_cid, piece_cid_d);
    start += piece_commitment_d_size;
    paths.push_back(piece_file_d_path);
    exist_pieces.push_back(piece_commitment_d_size);

    Path piece_file_e_path =
        boost::filesystem::unique_path(path_model).string();
    boost::filesystem::ofstream piece_file_e(piece_file_e_path);

    UnpaddedPieceSize piece_commitment_e_size(254);
    for (size_t i = start; i < start + piece_commitment_e_size; i++) {
      piece_file_e << some_bytes[i];
    }
    piece_file_e.close();

    EXPECT_OUTCOME_TRUE(
        piece_cid_e,
        proofs_->generatePieceCIDFromFile(
            seal_proof_type, piece_file_e_path, UnpaddedPieceSize(254)));

    PieceData file_e(piece_file_e_path);
    EXPECT_OUTCOME_TRUE(res_e,
                        proofs_->writeWithAlignment(seal_proof_type,
                                                    file_e,
                                                    piece_commitment_e_size,
                                                    unseal_path,
                                                    exist_pieces));

    ASSERT_EQ(res_e.total_write_unpadded, 254);
    ASSERT_EQ(res_e.piece_cid, piece_cid_e);
    paths.push_back(piece_file_e_path);
    exist_pieces.push_back(piece_commitment_e_size);

    PaddedPieceSize offset(0);
    for (size_t i = 0; i < paths.size(); i++) {
      int p[2];
      auto _ = gsl::finally([&p]() { close(p[0]); });

      ASSERT_FALSE(pipe(p) < 0);
      PieceData piece(p[1]);

      EXPECT_OUTCOME_TRUE_1(proofs_->readPiece(
          std::move(piece), unseal_path, offset, exist_pieces[i]));
      auto actual_piece = readFile(paths[i]);
      char ch;
      for (char j : actual_piece) {
        read(p[0], &ch, 1);
        EXPECT_EQ(ch, j);
      }
      offset = offset + exist_pieces[i].padded();
    }
  }

  /// update empty sector
  TEST_F(ProofsTest, Update) {
    const auto seal_type{RegisteredSealProof::kStackedDrg2KiBV1};
    const auto update_type{RegisteredUpdateProof::kStackedDrg2KiBV1};
    const PaddedPieceSize padded{2 << 10};

    const auto join{[&](auto s) { return (base_path / s).string(); }};
    const auto path_unsealed{join("unsealed")};
    const auto path_sealed{join("sealed")};
    const auto path_cache{join("cache")};
    const auto path_update{join("update")};
    const auto path_update_cache{join("update-cache")};

    touch(path_unsealed, 0);
    const std::vector<PieceInfo> pieces_old{{
        padded,
        sector_storage::zerocomm::getZeroPieceCommitment(padded.unpadded())
            .value(),
    }};
    touch(path_sealed, 0);
    fs::create_directory(path_cache);
    const auto precommit1{proofs_
                              ->sealPreCommitPhase1(seal_type,
                                                    path_cache,
                                                    path_unsealed,
                                                    path_sealed,
                                                    SectorNumber{},
                                                    ActorId{},
                                                    Ticket{},
                                                    pieces_old)
                              .value()};
    const auto cids_old{
        proofs_->sealPreCommitPhase2(precommit1, path_cache, path_sealed)
            .value()};

    const auto path_piece{join("piece")};
    std::string piece_str;
    piece_str.resize(padded.unpadded());
    std::generate(
        piece_str.begin(),
        piece_str.end(),
        std::independent_bits_engine<std::random_device, 8, uint8_t>{});
    fs::save_string_file(path_piece, piece_str);

    fs::remove(path_unsealed);
    const auto piece{proofs_
                         ->writeWithoutAlignment(seal_type,
                                                 PieceData{path_piece},
                                                 padded.unpadded(),
                                                 path_unsealed)
                         .value()};
    const std::vector<PieceInfo> pieces{{padded, piece.piece_cid}};

    touch(path_update, padded);
    fs::create_directory(path_update_cache);
    const auto cids{proofs_
                        ->updateSeal(update_type,
                                     path_update,
                                     path_update_cache,
                                     path_sealed,
                                     path_cache,
                                     path_unsealed,
                                     pieces)
                        .value()};
    const auto proofs1{proofs_
                           ->updateProve1(update_type,
                                          cids_old.sealed_cid,
                                          cids.sealed_cid,
                                          cids.unsealed_cid,
                                          path_update,
                                          path_update_cache,
                                          path_sealed,
                                          path_cache)
                           .value()};
    const auto proof{proofs_
                         ->updateProve2(update_type,
                                        cids_old.sealed_cid,
                                        cids.sealed_cid,
                                        cids.unsealed_cid,
                                        proofs1)
                         .value()};
    EXPECT_OUTCOME_EQ(proofs_->verifyUpdateProof({
                          update_type,
                          cids_old.sealed_cid,
                          cids.sealed_cid,
                          cids.unsealed_cid,
                          proof,
                      }),
                      true);

    std::string unsealed_file;
    fs::load_string_file(path_unsealed, unsealed_file);
    fs::remove(path_unsealed);
    touch(path_unsealed, padded);
    proofs_
        ->updateUnseal(update_type,
                       path_unsealed,
                       path_update,
                       path_sealed,
                       path_cache,
                       cids.unsealed_cid)
        .value();
    std::string unsealed_file2;
    fs::load_string_file(path_unsealed, unsealed_file2);
    EXPECT_EQ(unsealed_file2, unsealed_file);
  }
}  // namespace fc::proofs
