/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/basic_precommit_policy.hpp"

#include <gtest/gtest.h>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

namespace fc::mining {
  using primitives::block::BlockHeader;
  using primitives::piece::PieceInfo;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::TipsetError;
  using types::DealInfo;
  using vm::actor::builtin::types::miner::kMinSectorExpiration;
  using vm::actor::builtin::types::miner::kWPoStProvingPeriod;

  class PreCommitPolicyTest : public testing::Test {
   protected:
    virtual void SetUp() {
      api_ = std::make_shared<FullNodeApi>();

      sector_lifetime_ = std::chrono::seconds(30);
      proving_boundary_ = 100;

      precommit_policy_ = std::make_shared<BasicPreCommitPolicy>(
          api_, proving_boundary_, sector_lifetime_);
    }

    std::shared_ptr<FullNodeApi> api_;
    std::chrono::seconds sector_lifetime_;
    ChainEpoch proving_boundary_;
    std::shared_ptr<PreCommitPolicy> precommit_policy_;
  };

  /**
   * @given No pieces
   * @when try to get expiration epoch
   * @then get correct expiration(from Head)
   */
  TEST_F(PreCommitPolicyTest, ExpirationEmptyPieces) {
    BlockHeader block;
    block.height = kWPoStProvingPeriod;
    auto tipset = std::make_shared<Tipset>(TipsetKey(),
                                           std::vector<BlockHeader>({block}));

    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> {
      return outcome::success(tipset);
    };

    auto result = block.height + kMinSectorExpiration + kWPoStProvingPeriod;

    EXPECT_OUTCOME_EQ(precommit_policy_->expiration({}), result);
  }

  /**
   * @given 3 piece(without deal, expired deal, good deal)
   * @when try to get expiration epoch
   * @then get correct expiration(from last Piece)
   */
  TEST_F(PreCommitPolicyTest, Expiration) {
    BlockHeader block;
    block.height = kWPoStProvingPeriod;
    auto tipset = std::make_shared<Tipset>(TipsetKey(),
                                           std::vector<BlockHeader>({block}));

    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> {
      return outcome::success(tipset);
    };

    DealInfo deal1;
    deal1.deal_schedule.end_epoch =
        block.height + kMinSectorExpiration + kWPoStProvingPeriod + 2;
    PieceInfo piece1;
    piece1.cid = "010001020002"_cid;
    DealInfo deal2;
    deal2.deal_schedule.end_epoch = block.height + 1;
    std::vector<types::Piece> pieces = {{.piece = {}, .deal_info = boost::none},
                                        {.piece = piece1, .deal_info = deal1},
                                        {.piece = {}, .deal_info = deal2}};

    EXPECT_OUTCOME_EQ(
        precommit_policy_->expiration(gsl::span<const types::Piece>(pieces)),
        deal1.deal_schedule.end_epoch);
  }

  /**
   * @given broken api
   * @when try to get expiration epoch
   * @then error occurs
   */
  TEST_F(PreCommitPolicyTest, ExpirationApiError) {
    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> {
      return outcome::failure(TipsetError::kNoBlocks);
    };
    EXPECT_OUTCOME_ERROR(TipsetError::kNoBlocks,
                         precommit_policy_->expiration({}));
  }

}  // namespace fc::mining
