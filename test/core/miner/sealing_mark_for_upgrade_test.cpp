/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/miner/sealing_test_fixture.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"

namespace fc::mining {
  using primitives::ChainEpoch;
  using vm::actor::builtin::types::market::kDealMinDuration;

  class SealingMarkForUpgradeTest : public SealingTestFixture {
   protected:
    UnpaddedPieceSize piece_size{127};
    PieceData piece{"/dev/random"};
    PieceData piece2{"/dev/random"};
    DealInfo deal{
        .publish_cid = "010001020001"_cid,
        .deal_id = 0,
        .deal_schedule =
            {
                .start_epoch = 0,
                .end_epoch = 1,
            },
        .is_keep_unsealed = true,
    };
    SectorNumber sector{1};
    PieceInfo info1{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };
    PieceInfo info2{
        .size = piece_size.padded(),
        .cid = "010001020002"_cid,
    };
    SectorRef sector_ref{.id = SectorId{.miner = miner_id_, .sector = sector},
                         .proof_type = seal_proof_type_};
  };

  /**
   * @given sector(not in Proving state)
   * @when try to mark for snap upgrade
   * @then SealingError::kNotProvingState occurs
   */
  TEST_F(SealingMarkForUpgradeTest, MarkForSnapUpgradeNotProvingState) {
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));
    EXPECT_CALL(*manager_,
                doAddPieceSync(
                    sector_ref, IsEmpty(), piece_size, _, kDealSectorPriority))
        .WillOnce(testing::Return(info1));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, std::move(piece), deal));

    EXPECT_OUTCOME_ERROR(SealingError::kNotProvingState,
                         sealing_->markForSnapUpgrade(sector));
  }

  /**
   * @given sector(with several pieces)
   * @when try to mark for snap upgrade
   * @then SealingError::kUpgradeSeveralPieces occurs
   */
  TEST_F(SealingMarkForUpgradeTest, MarkForSnapUpgradeSeveralPieces) {
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));
    EXPECT_CALL(*manager_,
                doAddPieceSync(
                    sector_ref, IsEmpty(), piece_size, _, kDealSectorPriority))
        .WillOnce(testing::Return(info1));

    std::vector<UnpaddedPieceSize> exist_pieces({piece_size});
    EXPECT_CALL(
        *manager_,
        doAddPieceSync(
            sector_ref, exist_pieces, piece_size, _, kDealSectorPriority))
        .WillOnce(testing::Return(info2));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, std::move(piece), deal));
    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, std::move(piece2), deal));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->forceSectorState(sector, SealingState::kProving));

    runForSteps(*context_, 100);

    EXPECT_OUTCOME_ERROR(SealingError::kUpgradeSeveralPieces,
                         sealing_->markForSnapUpgrade(sector));
  }

  /**
   * @given sector(has deal)
   * @when try to mark for snap upgrade
   * @then SealingError::kUpgradeWithDeal occurs
   */
  TEST_F(SealingMarkForUpgradeTest, MarkForSnapUpgradeWithDeal) {
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));
    EXPECT_CALL(*manager_,
                doAddPieceSync(
                    sector_ref, IsEmpty(), piece_size, _, kDealSectorPriority))
        .WillOnce(testing::Return(info1));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, std::move(piece), deal));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->forceSectorState(sector, SealingState::kProving));

    runForSteps(*context_, 100);

    EXPECT_OUTCOME_ERROR(SealingError::kUpgradeWithDeal,
                         sealing_->markForSnapUpgrade(sector));
  }

  /**
   * @given sector not in miner active sectors
   * @when try to mark for snap upgrade
   * @then return error SealingError::kCannotMarkInactiveSector
   */
  TEST_F(SealingMarkForUpgradeTest, MarkForSnapUpgradeActiveSector) {
    TipsetKey key{{CbCid::hash("01"_unhex)}};
    auto head = std::make_shared<Tipset>(key, std::vector<BlockHeader>());
    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> { return head; };
    api_->StateMinerActiveSectors = [&](const Address &, const TipsetKey &)
        -> outcome::result<std::vector<SectorOnChainInfo>> {
      // doesn't contain upgraded sector
      return std::vector<SectorOnChainInfo>{};
    };

    EXPECT_OUTCOME_ERROR(SealingError::kCannotMarkInactiveSector,
                         sealing_->markForSnapUpgrade(update_sector_id_));
  }

  /**
   * @given sector is due to expire
   * @when try to mark for snap upgrade
   * @then SealingError::kSectorExpirationError occurs
   */
  TEST_F(SealingMarkForUpgradeTest, MarkForSnapUpgradeExpired) {
    ChainEpoch height = 100;
    TipsetKey key{{CbCid::hash("01"_unhex)}};
    auto head = std::make_shared<Tipset>(
        key, std::vector<BlockHeader>{BlockHeader{.height = height}});
    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> { return head; };
    api_->StateMinerActiveSectors = [&](const Address &, const TipsetKey &)
        -> outcome::result<std::vector<SectorOnChainInfo>> {
      // contains upgraded sector
      return std::vector<SectorOnChainInfo>{
          SectorOnChainInfo{.sector = update_sector_id_}};
    };
    api_->StateSectorGetInfo =
        [&](const Address &, SectorNumber, const TipsetKey &key)
        -> outcome::result<boost::optional<SectorOnChainInfo>> {
      return SectorOnChainInfo{.expiration = height};
    };

    EXPECT_OUTCOME_ERROR(SealingError::kSectorExpirationError,
                         sealing_->markForSnapUpgrade(update_sector_id_));
  }

}  // namespace fc::mining
