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
  PieceInfo piece_info{.deal_id = 1, .sector_id = 2, .offset = 3, .length = 4};
  PayloadLocation location_A{.relative_offset = 0, .block_size = 100};
  PayloadLocation location_B{.relative_offset = 100, .block_size = 50};

  PieceStorageTest()
      : storage_backend{std::make_shared<InMemoryStorage>()},
        piece_storage{std::make_shared<PieceStorageImpl>(storage_backend)} {}
};

/**
 * @given Example Piece CID and Piece info
 * @when Writing and retrieving Piece info
 * @then All operations must be successful and retrieved info must be the same
 */
TEST_F(PieceStorageTest, AddPieceInfoSuccess) {
  EXPECT_OUTCOME_TRUE_1(piece_storage->addPieceInfo(piece_cid, piece_info))
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
  EXPECT_OUTCOME_TRUE(payload_location_A,
                      piece_storage->getPayloadLocation(payload_cid_A));
  ASSERT_EQ(payload_location_A.parent_piece, piece_cid);
  ASSERT_EQ(payload_location_A.block_location, location_A);
  EXPECT_OUTCOME_TRUE(payload_location_B,
                      piece_storage->getPayloadLocation(payload_cid_B));
  ASSERT_EQ(payload_location_B.parent_piece, piece_cid);
  ASSERT_EQ(payload_location_B.block_location, location_B);
}
