/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/piece/impl/piece_storage_impl.hpp"

#include <memory>

#include <gtest/gtest.h>
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace fc::storage::piece;
using fc::CID;
using fc::storage::InMemoryStorage;

struct PieceStorageTest : public ::testing::Test {
  std::shared_ptr<InMemoryStorage> storage_backend;
  std::shared_ptr<PieceStorage> piece_storage;
  CID piece_cid{"010001020001"_cid};
  CID payload_cid_A{"010001020002"_cid};
  CID payload_cid_B{"010001020003"_cid};
  DealInfo deal_info{.deal_id = 1, .sector_id = 2, .offset = 3, .length = 4};
  PieceInfo piece_info{.piece_cid = piece_cid, .deals = {deal_info}};
  PayloadLocation location_A{.relative_offset = 0, .block_size = 100};
  PayloadLocation location_B{.relative_offset = 100, .block_size = 50};

  PieceStorageTest()
      : storage_backend{std::make_shared<InMemoryStorage>()},
        piece_storage{std::make_shared<PieceStorageImpl>(storage_backend)} {}
};

/**
 * @given Example Piece CID and empty storage
 * @when retrieving nonexisting Piece info
 * @then return error piece info not found
 */
TEST_F(PieceStorageTest, GetPieceInfoNotFound){
    EXPECT_OUTCOME_ERROR(PieceStorageError::kPieceNotFound,
                         piece_storage->getPieceInfo(piece_cid))}

/**
 * @given Example Piece CID and Piece info
 * @when Writing and retrieving Piece info
 * @then All operations must be successful and retrieved info must be the same
 */
TEST_F(PieceStorageTest, AddPieceInfoSuccess) {
  EXPECT_OUTCOME_TRUE_1(piece_storage->addDealForPiece(piece_cid, deal_info))
  EXPECT_OUTCOME_TRUE(received_info, piece_storage->getPieceInfo(piece_cid))
  ASSERT_EQ(received_info, piece_info);
}

/**
 * @given Exaple Piece CID and blocks locations
 * @when Writing and retrieving block locations
 * @then All operations must be successful and retrieved info must be the same
 */
TEST_F(PieceStorageTest, AddBlockLocationSuccess) {
  std::map<CID, PayloadLocation> locations{{payload_cid_A, location_A},
                                           {payload_cid_B, location_B}};
  EXPECT_OUTCOME_TRUE_1(
      piece_storage->addPayloadLocations(piece_cid, locations));

  EXPECT_OUTCOME_TRUE(payload_info_A,
                      piece_storage->getPayloadInfo(payload_cid_A));
  EXPECT_EQ(payload_info_A.cid, payload_cid_A);
  EXPECT_EQ(payload_info_A.piece_block_locations.size(), 1);
  EXPECT_EQ(payload_info_A.piece_block_locations.front().parent_piece,
            piece_cid);
  EXPECT_EQ(payload_info_A.piece_block_locations.front().block_location,
            location_A);

  EXPECT_OUTCOME_TRUE(payload_info_B,
                      piece_storage->getPayloadInfo(payload_cid_B));
  EXPECT_EQ(payload_info_B.cid, payload_cid_B);
  EXPECT_EQ(payload_info_B.piece_block_locations.size(), 1);
  EXPECT_EQ(payload_info_B.piece_block_locations.front().parent_piece,
            piece_cid);
  EXPECT_EQ(payload_info_B.piece_block_locations.front().block_location,
            location_B);
}
