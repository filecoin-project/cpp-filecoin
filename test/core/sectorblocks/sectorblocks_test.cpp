/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sectorblocks/impl/blocks_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "testutil/mocks/miner/miner_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::sectorblocks {
  using api::PieceLocation;
  using miner::MinerMock;
  using testing::_;

  class SectorBlocksTest : public ::testing::Test {
   protected:
    void SetUp() override {
      miner_ = std::make_unique<MinerMock>();

      sector_blocks_ = std::make_unique<SectorBlocksImpl>(miner_);
    }

    std::shared_ptr<MinerMock> miner_;
    std::shared_ptr<SectorBlocks> sector_blocks_;
  };

  TEST_F(SectorBlocksTest, GetMiner) {
    ASSERT_EQ(miner_, sector_blocks_->getMiner());
  }

  TEST_F(SectorBlocksTest, NotFoundSector) {
    DealId deal_id = 1;

    EXPECT_OUTCOME_ERROR(Error::kNotFound, sector_blocks_->getRefs(deal_id))
  }

  TEST_F(SectorBlocksTest, AddRefs) {
    DealInfo deal{
        .publish_cid = boost::none,
        .deal_id = 1,
        .deal_schedule =
            {
                .start_epoch = 10,
                .end_epoch = 11,
            },
        .is_keep_unsealed = false,
    };
    UnpaddedPieceSize size(127);
    std::string path = "/some/temp/path";

    PieceAttributes piece{
        .sector = 1,
        .offset = PaddedPieceSize(0),
        .size = UnpaddedPieceSize(127),
    };

    EXPECT_CALL(*miner_, addPieceToAnySector(size, _, deal))
        .WillOnce(testing::Return(outcome::success(piece)));

    EXPECT_OUTCOME_EQ(sector_blocks_->addPiece(size, path, deal), piece);

    PieceLocation result_ref{
        .sector_number = piece.sector,
        .offset = piece.offset,
        .length = piece.size.padded(),
    };

    EXPECT_OUTCOME_TRUE(refs, sector_blocks_->getRefs(deal.deal_id))

    ASSERT_THAT(refs, testing::ElementsAre(result_ref));
  }

}  // namespace fc::sectorblocks
