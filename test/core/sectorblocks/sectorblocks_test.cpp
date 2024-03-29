/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sectorblocks/impl/blocks_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/mocks/miner/miner_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::sectorblocks {
  using api::PieceLocation;
  using miner::MinerMock;
  using storage::InMemoryStorage;
  using testing::_;

  class SectorBlocksTest : public ::testing::Test {
   protected:
    void SetUp() override {
      miner_ = std::make_unique<MinerMock>();
      datastore_ = std::make_shared<InMemoryStorage>();
      sector_blocks_ = std::make_unique<SectorBlocksImpl>(miner_, datastore_);
    }

    std::shared_ptr<MinerMock> miner_;
    std::shared_ptr<SectorBlocks> sector_blocks_;
    std::shared_ptr<InMemoryStorage> datastore_;
  };

  /**
   * @given sectorblocks
   * @when try to get miner
   * @then the miner is returned
   */
  TEST_F(SectorBlocksTest, GetMiner) {
    ASSERT_EQ(miner_, sector_blocks_->getMiner());
  }

  /**
   * @given sectorblocks and non exist deal id
   * @when try to get refs with the id
   * @then SectorBlocksError::kNotFoundDeal error occurs
   */
  TEST_F(SectorBlocksTest, NotFoundSector) {
    DealId deal_id = 1;

    EXPECT_OUTCOME_ERROR(SectorBlocksError::kNotFoundDeal,
                         sector_blocks_->getRefs(deal_id))
  }

  /**
   * @given  sectorblocks, deal, size, and path
   * @when try to add piece and then get refs
   * @then success
   */
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

    PieceLocation piece{
        .sector = 1,
        .offset = PaddedPieceSize(0),
        .size = PaddedPieceSize(128),
    };

    EXPECT_CALL(*miner_, doAddPieceToAnySector(size, _, deal))
        .WillOnce(testing::Return(outcome::success(piece)));

    EXPECT_OUTCOME_EQ(sector_blocks_->addPiece(size, path, deal), piece);

    EXPECT_OUTCOME_TRUE(refs, sector_blocks_->getRefs(deal.deal_id))

    ASSERT_THAT(refs, testing::ElementsAre(piece));
  }

  /**
   * @given  sectorblocks, deal, size, and path
   * @when try to add two duplicate pieces to the same deal_id
   * @then EXPECT_OUTCOME_ERROR
   */
  TEST_F(SectorBlocksTest, DuplicatePiece) {
    const DealInfo deal{
        .publish_cid = boost::none,
        .deal_id = 1,
        .deal_schedule =
            {
                .start_epoch = 10,
                .end_epoch = 11,
            },
        .is_keep_unsealed = false,
    };
    const UnpaddedPieceSize size(127);
    const std::string path = "/some/temp/path";

    const PieceLocation piece{
        .sector = 1,
        .offset = PaddedPieceSize(0),
        .size = PaddedPieceSize(128),
    };

    EXPECT_CALL(*miner_, doAddPieceToAnySector(size, _, deal))
        .WillOnce(testing::Return(outcome::success(piece)))
        .WillOnce(testing::Return(outcome::success(piece)));

    EXPECT_OUTCOME_EQ(sector_blocks_->addPiece(size, path, deal), piece);
    EXPECT_OUTCOME_ERROR(SectorBlocksError::kDealAlreadyExist,
                         sector_blocks_->addPiece(size, path, deal));
  }

}  // namespace fc::sectorblocks
