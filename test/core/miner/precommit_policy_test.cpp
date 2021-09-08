/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/basic_precommit_policy.hpp"

#include <gtest/gtest.h>

#include "testutil/literals.hpp"

namespace fc::mining {
  using primitives::block::BlockHeader;
  using primitives::piece::PieceInfo;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using primitives::tipset::TipsetError;
  using types::DealInfo;
  using vm::actor::builtin::types::miner::kWPoStProvingPeriod;

  class PreCommitPolicyTest : public testing::Test {
   protected:
    virtual void SetUp() {
      api_ = std::make_shared<FullNodeApi>();

      duration_ = 1;
      proving_boundary_ = 2;

      precommit_policy_ = std::make_shared<BasicPreCommitPolicy>(
          api_, proving_boundary_, duration_);
    }

    std::shared_ptr<FullNodeApi> api_;
    ChainEpoch duration_;
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

    auto result = block.height + kWPoStProvingPeriod + proving_boundary_ - 1;

    ASSERT_EQ(precommit_policy_->expiration({}), result);
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
    deal1.deal_schedule.end_epoch = block.height - 1;
    PieceInfo piece1;
    piece1.cid = "010001020002"_cid;
    DealInfo deal2;
    deal2.deal_schedule.end_epoch = block.height + 1;
    std::vector<types::Piece> pieces = {{.piece = {}, .deal_info = boost::none},
                                        {.piece = piece1, .deal_info = deal1},
                                        {.piece = {}, .deal_info = deal2}};

    auto result = block.height + kWPoStProvingPeriod + proving_boundary_ - 1;
    ASSERT_EQ(
        precommit_policy_->expiration(gsl::span<const types::Piece>(pieces)),
        result);
  }

  /**
   * @given broken api
   * @when try to get expiration epoch
   * @then 0 recived
   */
  TEST_F(PreCommitPolicyTest, ExpirationApiError) {
    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> {
      return outcome::failure(TipsetError::kNoBlocks);
    };
    ASSERT_EQ(precommit_policy_->expiration({}), 0);
  }

}  // namespace fc::mining
