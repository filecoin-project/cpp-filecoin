/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sectorblocks/impl/blocks_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "testutil/mocks/miner/miner_mock.hpp"
#include "testutil/mocks/sectorblocks/blocks_mock.hpp"
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

  TEST(Experiment, OneS) {
    std::shared_ptr<SectorBlocksMock> bloks =
        std::make_shared<SectorBlocksMock>();

    EXPECT_CALL(*bloks, getRefs(1))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(test1(bloks));
  }
  TEST(Experiment, OneF) {
    std::shared_ptr<SectorBlocksMock> bloks =
        std::make_shared<SectorBlocksMock>();

    EXPECT_CALL(*bloks, getRefs(1))
        .WillOnce(testing::Return(outcome::failure(ERROR_TEXT("ERROR"))));

    ASSERT_TRUE(test1(bloks).has_error());
  }
  TEST(Experiment, Two) {
    std::shared_ptr<SectorBlocksMock> bloks =
        std::make_shared<SectorBlocksMock>();

    EXPECT_CALL(*bloks, getRefs(1))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(test2(bloks));
  }
  TEST(Experiment, Three) {
    std::shared_ptr<SectorBlocksMock> bloks =
        std::make_shared<SectorBlocksMock>();

    EXPECT_CALL(*bloks, getRefs(1))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(test3(bloks));
  }
  TEST(Experiment, Four) {
    std::shared_ptr<SectorBlocksMock> bloks =
        std::make_shared<SectorBlocksMock>();

    EXPECT_CALL(*bloks, getRefs(1))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(test4(bloks));
  }
  TEST(Experiment, Five) {
    std::shared_ptr<SectorBlocksMock> bloks =
        std::make_shared<SectorBlocksMock>();

    EXPECT_CALL(*bloks, getRefs(1))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(test5(bloks));
  }
  TEST(Experiment, Six) {
    std::shared_ptr<SectorBlocksMock> bloks =
        std::make_shared<SectorBlocksMock>();

    EXPECT_CALL(*bloks, getRefs(1))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(test6(bloks));
  }
  TEST(Experiment, Seven) {
    EXPECT_OUTCOME_TRUE_1(test7());
  }

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
