/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/sealing_impl.hpp"

#include <gtest/gtest.h>
#include <chrono>

#include "primitives/sector/sector.hpp"
#include "testutil/context_wait.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/miner/events_mock.hpp"
#include "testutil/mocks/miner/precommit_policy_mock.hpp"
#include "testutil/mocks/primitives/stored_counter_mock.hpp"
#include "testutil/mocks/sector_storage/manager_mock.hpp"
#include "testutil/mocks/storage/buffer_map_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::mining {

  using api::MinerInfo;
  using primitives::CounterMock;
  using sector_storage::ManagerMock;
  using storage::BufferMapMock;
  using testing::_;
  using types::kDealSectorPriority;

  class SealingTest : public testing::Test {
   protected:
    void SetUp() override {
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;
      auto sector_size = primitives::sector::getSectorSize(seal_proof_type_);
      ASSERT_FALSE(sector_size.has_error());
      sector_size_ = PaddedPieceSize(sector_size.value());

      api_ = std::make_shared<FullNodeApi>();
      events_ = std::make_shared<EventsMock>();
      miner_id_ = 42;
      miner_addr_ = Address::makeFromId(miner_id_);
      counter_ = std::make_shared<CounterMock>();
      kv_ = std::make_shared<BufferMapMock>();

      EXPECT_CALL(*kv_, doPut(_, _))
          .WillRepeatedly(testing::Return(outcome::success()));

      manager_ = std::make_shared<ManagerMock>();

      EXPECT_CALL(*manager_, getSectorSize())
          .WillRepeatedly(testing::Return(sector_size_));

      policy_ = std::make_shared<PreCommitPolicyMock>();
      context_ = std::make_shared<boost::asio::io_context>();

      config_ = Config{
          .max_wait_deals_sectors = 2,
          .max_sealing_sectors = 0,
          .max_sealing_sectors_for_deals = 0,
          .wait_deals_delay = 0  // by default 6 hours
      };

      EXPECT_CALL(*kv_, cursor())
          .WillOnce(testing::Return(testing::ByMove(nullptr)));

      EXPECT_OUTCOME_TRUE(sealing,
                          SealingImpl::newSealing(api_,
                                                  events_,
                                                  miner_addr_,
                                                  counter_,
                                                  kv_,
                                                  manager_,
                                                  policy_,
                                                  context_,
                                                  config_));
      sealing_ = sealing;
    }

    void TearDown() override {
      context_->stop();
    }

    RegisteredSealProof seal_proof_type_;

    PaddedPieceSize sector_size_;
    Config config_;
    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<EventsMock> events_;
    uint64_t miner_id_;
    Address miner_addr_;
    std::shared_ptr<CounterMock> counter_;
    std::shared_ptr<BufferMapMock> kv_;
    std::shared_ptr<ManagerMock> manager_;
    std::shared_ptr<PreCommitPolicyMock> policy_;
    std::shared_ptr<boost::asio::io_context> context_;

    std::shared_ptr<Sealing> sealing_;
  };

  TEST_F(SealingTest, getAddress) {
    ASSERT_EQ(miner_addr_, sealing_->getAddress());
  }

  TEST_F(SealingTest, getSectorInfoNotFound) {
    EXPECT_OUTCOME_ERROR(SealingError::kCannotFindSector,
                         sealing_->getSectorInfo(1));
  }

  TEST_F(SealingTest, RemoveNotFound) {
    EXPECT_OUTCOME_ERROR(SealingError::kCannotFindSector, sealing_->remove(1));
  }

  TEST_F(SealingTest, Remove) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
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

    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    api_->StateMinerInfo =
        [&](const Address &address,
            const TipsetKey &tipset_key) -> outcome::result<MinerInfo> {
      if (address == miner_addr_) {
        MinerInfo info;
        info.seal_proof_type = seal_proof_type_;
        return info;
      }
      return SealingError::kPieceNotFit;  // some error
    };

    PieceInfo info{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };

    EXPECT_CALL(*manager_,
                addPiece(SectorId{.miner = miner_id_, .sector = sector},
                         gsl::span<const UnpaddedPieceSize>(),
                         piece_size,
                         _,
                         kDealSectorPriority))
        .WillOnce(testing::Return(info));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, piece, deal));

    EXPECT_OUTCOME_TRUE(info_before, sealing_->getSectorInfo(sector));
    EXPECT_EQ(info_before->state, SealingState::kStateUnknown);
    EXPECT_OUTCOME_TRUE_1(
        sealing_->forceSectorState(sector, SealingState::kProving));
    EXPECT_OUTCOME_TRUE_1(sealing_->remove(sector));

    EXPECT_CALL(*manager_,
                remove(SectorId{.miner = miner_id_, .sector = sector}))
        .WillOnce(testing::Return(outcome::success()));

    runForSteps(*context_, 100);

    EXPECT_OUTCOME_TRUE(sector_info, sealing_->getSectorInfo(sector));
    EXPECT_EQ(sector_info->state, SealingState::kRemoved);
  }

  TEST_F(SealingTest, addPieceToAnySectorNotPublishedDeal) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
    DealInfo deal{
        .publish_cid = boost::none,
        .deal_id = 0,
        .deal_schedule =
            {
                .start_epoch = 0,
                .end_epoch = 1,
            },
        .is_keep_unsealed = true,
    };
    EXPECT_OUTCOME_ERROR(
        SealingError::kNotPublishedDeal,
        sealing_->addPieceToAnySector(piece_size, piece, deal));
  }

  TEST_F(SealingTest, addPieceToAnySectorCannotAllocatePiece) {
    UnpaddedPieceSize piece_size(128);
    PieceData piece("/dev/random");
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
    EXPECT_OUTCOME_ERROR(
        SealingError::kCannotAllocatePiece,
        sealing_->addPieceToAnySector(piece_size, piece, deal));
  }

  TEST_F(SealingTest, addPieceToAnySectorPieceNotFit) {
    UnpaddedPieceSize piece_size(4064);
    PieceData piece("/dev/random");
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

    EXPECT_OUTCOME_ERROR(
        SealingError::kPieceNotFit,
        sealing_->addPieceToAnySector(piece_size, piece, deal));
  }

  TEST_F(SealingTest, addPieceToAnySectorWithoutStartPacking) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
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

    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    api_->StateMinerInfo =
        [&](const Address &address,
            const TipsetKey &tipset_key) -> outcome::result<MinerInfo> {
      if (address == miner_addr_) {
        MinerInfo info;
        info.seal_proof_type = seal_proof_type_;
        return info;
      }
      return SealingError::kPieceNotFit;  // some error
    };

    PieceInfo info{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };

    EXPECT_CALL(*manager_,
                addPiece(SectorId{.miner = miner_id_, .sector = sector},
                         gsl::span<const UnpaddedPieceSize>(),
                         piece_size,
                         _,
                         kDealSectorPriority))
        .WillOnce(testing::Return(info));

    EXPECT_OUTCOME_TRUE(piece_attribute,
                        sealing_->addPieceToAnySector(piece_size, piece, deal));
    EXPECT_EQ(piece_attribute.sector, sector);
    EXPECT_EQ(piece_attribute.offset, 0);
    EXPECT_EQ(piece_attribute.size, piece_size);

    runForSteps(*context_, 100);

    EXPECT_OUTCOME_TRUE(sector_info, sealing_->getSectorInfo(sector));
    EXPECT_EQ(sector_info->sector_number, sector);
    EXPECT_EQ(sector_info->state, SealingState::kWaitDeals);
  }

  TEST_F(SealingTest, MarkForUpgradeNotProvingState) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
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

    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    api_->StateMinerInfo =
        [&](const Address &address,
            const TipsetKey &tipset_key) -> outcome::result<MinerInfo> {
      if (address == miner_addr_) {
        MinerInfo info;
        info.seal_proof_type = seal_proof_type_;
        return info;
      }
      return SealingError::kPieceNotFit;  // some error
    };

    PieceInfo info{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };

    EXPECT_CALL(*manager_,
                addPiece(SectorId{.miner = miner_id_, .sector = sector},
                         gsl::span<const UnpaddedPieceSize>(),
                         piece_size,
                         _,
                         kDealSectorPriority))
        .WillOnce(testing::Return(info));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, piece, deal));

    EXPECT_OUTCOME_ERROR(SealingError::kNotProvingState,
                         sealing_->markForUpgrade(sector));
  }

  TEST_F(SealingTest, MarkForUpgradeSeveralPieces) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
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

    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    api_->StateMinerInfo =
        [&](const Address &address,
            const TipsetKey &tipset_key) -> outcome::result<MinerInfo> {
      if (address == miner_addr_) {
        MinerInfo info;
        info.seal_proof_type = seal_proof_type_;
        return info;
      }
      return SealingError::kPieceNotFit;  // some error
    };

    PieceInfo info1{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };
    PieceInfo info2{
        .size = piece_size.padded(),
        .cid = "010001020002"_cid,
    };

    EXPECT_CALL(*manager_,
                addPiece(SectorId{.miner = miner_id_, .sector = sector},
                         gsl::span<const UnpaddedPieceSize>(),
                         piece_size,
                         _,
                         kDealSectorPriority))
        .WillOnce(testing::Return(info1));

    EXPECT_CALL(*manager_,
                addPiece(SectorId{.miner = miner_id_, .sector = sector},
                         gsl::span<const UnpaddedPieceSize>({piece_size}),
                         piece_size,
                         _,
                         kDealSectorPriority))
        .WillOnce(testing::Return(info2));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, piece, deal));
    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, piece, deal));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->forceSectorState(sector, SealingState::kProving));

    runForSteps(*context_, 100);

    EXPECT_OUTCOME_ERROR(SealingError::kUpgradeSeveralPieces,
                         sealing_->markForUpgrade(sector));
  }

  TEST_F(SealingTest, MarkForUpgradeWitdDeal) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
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

    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    api_->StateMinerInfo =
        [&](const Address &address,
            const TipsetKey &tipset_key) -> outcome::result<MinerInfo> {
      if (address == miner_addr_) {
        MinerInfo info;
        info.seal_proof_type = seal_proof_type_;
        return info;
      }
      return SealingError::kPieceNotFit;  // some error
    };

    PieceInfo info{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };

    EXPECT_CALL(*manager_,
                addPiece(SectorId{.miner = miner_id_, .sector = sector},
                         gsl::span<const UnpaddedPieceSize>(),
                         piece_size,
                         _,
                         kDealSectorPriority))
        .WillOnce(testing::Return(info));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, piece, deal));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->forceSectorState(sector, SealingState::kProving));

    runForSteps(*context_, 100);

    EXPECT_OUTCOME_ERROR(SealingError::kUpgradeWithDeal,
                         sealing_->markForUpgrade(sector));
  }

  TEST_F(SealingTest, ListOfSectors) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
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

    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    api_->StateMinerInfo =
        [&](const Address &address,
            const TipsetKey &tipset_key) -> outcome::result<MinerInfo> {
      if (address == miner_addr_) {
        MinerInfo info;
        info.seal_proof_type = seal_proof_type_;
        return info;
      }
      return SealingError::kPieceNotFit;  // some error
    };

    PieceInfo info{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };

    EXPECT_CALL(*manager_,
                addPiece(SectorId{.miner = miner_id_, .sector = sector},
                         gsl::span<const UnpaddedPieceSize>(),
                         piece_size,
                         _,
                         kDealSectorPriority))
        .WillOnce(testing::Return(info));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, piece, deal));

    auto sectors = sealing_->getListSectors();
    ASSERT_EQ(sectors.size(), 1);
  }

}  // namespace fc::mining
