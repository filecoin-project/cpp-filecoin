/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/sector_file/sector_file.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::primitives::sector_file {

  class SectorFileTest : public test::BaseFS_Test {
   public:
    SectorFileTest() : test::BaseFS_Test("fc_sector_file_test") {}

   protected:
    RegisteredSealProof min_seal_proof_type =
        RegisteredSealProof::kStackedDrg2KiBV1;
    RegisteredSealProof border_seal_proof_type =
        RegisteredSealProof::kStackedDrg8MiBV1;
    SectorFileType file_type = SectorFileType::FTCache;
  };

  /**
   * @given Sector
   * @when create sector file
   * @then file created and it's empty
   */
  TEST_F(SectorFileTest, CreatedFileEmpty) {
    EXPECT_OUTCOME_TRUE(
        sector_size,
        fc::primitives::sector::getSectorSize(min_seal_proof_type));
    SectorId sector{
        .miner = 1,
        .sector = 1,
    };

    EXPECT_OUTCOME_TRUE(
        file,
        SectorFile::createFile(
            (base_path / fc::primitives::sector_file::sectorName(sector))
                .string(),
            PaddedPieceSize(sector_size)));

    EXPECT_OUTCOME_EQ(
        file->hasAllocated(0, PaddedPieceSize(sector_size).unpadded()), false);
  }

  /**
   * @given SectorFile, reference cid
   * @when try to write small piece
   * @then piece is written and cids are equal
   */
  TEST_F(SectorFileTest, WriteSmallPiece) {
    EXPECT_OUTCOME_TRUE(
        sector_size,
        fc::primitives::sector::getSectorSize(min_seal_proof_type));
    SectorId sector{
        .miner = 1,
        .sector = 1,
    };

    PaddedPieceSize piece_size(128);
    std::string result_cid =
        "baga6ea4seaqpbf2qq3cpiezjszxd5tb7mnyeyf72tmuot3b6b556tpb762b3uoi";

    EXPECT_OUTCOME_TRUE(
        file,
        SectorFile::createFile(
            (base_path / fc::primitives::sector_file::sectorName(sector))
                .string(),
            PaddedPieceSize(sector_size)));

    EXPECT_OUTCOME_TRUE(
        piece_info,
        file->write(PieceData(resourcePath("payload.txt").string()),
                    0,
                    piece_size,
                    min_seal_proof_type));

    ASSERT_EQ(piece_info->size, piece_size);
    EXPECT_OUTCOME_EQ(piece_info->cid.toString(), result_cid);
    EXPECT_OUTCOME_EQ(file->hasAllocated(0, piece_size.unpadded()), true);
  }

  /**
   * @given SectorFile, reference cid
   * @when try to write piece with size more that 1 default piece(checks cids
   * combine)
   * @then piece is written and cids are equal
   */
  TEST_F(SectorFileTest, Write2ChunkPiece) {
    EXPECT_OUTCOME_TRUE(
        sector_size,
        fc::primitives::sector::getSectorSize(border_seal_proof_type));
    SectorId sector{
        .miner = 1,
        .sector = 1,
    };

    PaddedPieceSize piece_size(4194432);  // 1mb + 128 byte (by chunks)
    std::string result_cid =
        "baga6ea4seaqpyjg4wl5r7sblmrvzugvqr3nxv53lf2basmnppolirbjnrleosiy";

    EXPECT_OUTCOME_TRUE(
        file,
        SectorFile::createFile(
            (base_path / fc::primitives::sector_file::sectorName(sector))
                .string(),
            PaddedPieceSize(sector_size)));

    EXPECT_OUTCOME_TRUE(
        piece_info,
        file->write(PieceData(resourcePath("unpad_medium_file.txt").string()),
                    0,
                    piece_size,
                    border_seal_proof_type));

    ASSERT_EQ(piece_info->size, piece_size);
    EXPECT_OUTCOME_EQ(piece_info->cid.toString(), result_cid);
    EXPECT_OUTCOME_EQ(file->hasAllocated(0, piece_size.unpadded()), true);
  }

  /**
   * @given SectorFile, 2 piece
   * @when try to write 2 piece with some blank between
   * @then pieces are written
   */
  TEST_F(SectorFileTest, WritePieceWtihBlank) {
    EXPECT_OUTCOME_TRUE(
        sector_size,
        fc::primitives::sector::getSectorSize(min_seal_proof_type));
    SectorId sector{
        .miner = 1,
        .sector = 1,
    };

    PaddedPieceSize offset(1024);
    PaddedPieceSize piece_size(128);

    EXPECT_OUTCOME_TRUE(
        file,
        SectorFile::createFile(
            (base_path / fc::primitives::sector_file::sectorName(sector))
                .string(),
            PaddedPieceSize(sector_size)));

    EXPECT_OUTCOME_TRUE_1(file->write(
        PieceData(resourcePath("payload.txt").string()), 0, piece_size));

    EXPECT_OUTCOME_TRUE_1(file->write(
        PieceData(resourcePath("payload.txt").string()), offset, piece_size));

    EXPECT_OUTCOME_EQ(file->hasAllocated(0, piece_size.unpadded()), true);
    EXPECT_OUTCOME_EQ(
        file->hasAllocated(offset.unpadded(), piece_size.unpadded()), true);
  }

  /**
   * @given SectorFile with piece
   * @when try to read piece
   * @then piece is readed and correct
   */
  TEST_F(SectorFileTest, ReadPiece) {
    EXPECT_OUTCOME_TRUE(
        sector_size,
        fc::primitives::sector::getSectorSize(min_seal_proof_type));
    SectorId sector{
        .miner = 1,
        .sector = 1,
    };

    PaddedPieceSize piece_size(128);

    EXPECT_OUTCOME_TRUE(
        file,
        SectorFile::createFile(
            (base_path / fc::primitives::sector_file::sectorName(sector))
                .string(),
            PaddedPieceSize(sector_size)));

    EXPECT_OUTCOME_TRUE_1(file->write(
        PieceData(resourcePath("payload.txt").string()), 0, piece_size));

    auto result_data = readFile(resourcePath("payload.txt").string());

    result_data.resize(piece_size.unpadded());

    auto read_file_path = base_path / fs::unique_path();
    EXPECT_OUTCOME_TRUE_1(file->read(
        PieceData(read_file_path.string(), O_WRONLY | O_CREAT), 0, piece_size));
    const auto read_data = readFile(read_file_path);

    ASSERT_EQ(read_data.size(), piece_size.unpadded());
    ASSERT_EQ(read_data, result_data);
  }

  /**
   * @given SectorFile with piece
   * @when try to free piece
   * @then piece is released
   */
  TEST_F(SectorFileTest, FreePiece) {
    EXPECT_OUTCOME_TRUE(
        sector_size,
        fc::primitives::sector::getSectorSize(min_seal_proof_type));
    SectorId sector{
        .miner = 1,
        .sector = 1,
    };

    PaddedPieceSize piece_size(128);

    EXPECT_OUTCOME_TRUE(
        file,
        SectorFile::createFile(
            (base_path / fc::primitives::sector_file::sectorName(sector))
                .string(),
            PaddedPieceSize(sector_size)));

    EXPECT_OUTCOME_TRUE_1(file->write(
        PieceData(resourcePath("payload.txt").string()), 0, piece_size));

    auto read_file_path = base_path / fs::unique_path();
    EXPECT_OUTCOME_EQ(file->hasAllocated(0, piece_size.unpadded()), true);
    EXPECT_OUTCOME_TRUE_1(file->free(0, piece_size));
    EXPECT_OUTCOME_EQ(file->hasAllocated(0, piece_size.unpadded()), false);
  }

  /**
   * @given Seal Proof type and Sector File type
   * @when try to get amount of used memory for sealing
   * @then get amount of used memory for this configuration
   */
  TEST_F(SectorFileTest, SealSpaceUse) {
    EXPECT_OUTCOME_TRUE(
        sector_size,
        fc::primitives::sector::getSectorSize(min_seal_proof_type));
    uint64_t result =
        kOverheadSeal.at(file_type) * sector_size / kOverheadDenominator;
    EXPECT_OUTCOME_TRUE(seal_size,
                        sealSpaceUse(file_type, sector_size));
    ASSERT_EQ(result, seal_size);
  }
}  // namespace fc::primitives::sector_file
