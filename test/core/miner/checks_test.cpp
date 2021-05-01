/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/checks.hpp"

#include <gtest/gtest.h>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

namespace fc::mining::checks {
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using types::DealInfo;
  using types::DealSchedule;
  using types::PaddedPieceSize;
  using types::Piece;
  using types::PieceInfo;

  class CheckPieces : public testing::Test {
   protected:
    void SetUp() override {
      miner_id_ = 42;
      miner_addr_ = Address::makeFromId(miner_id_);

      api_ = std::make_shared<FullNodeApi>();
    }

    ActorId miner_id_;
    Address miner_addr_;
    std::shared_ptr<FullNodeApi> api_;
  };

  /**
   * @given filler piece with wrong CID
   * @when try to check pieces
   * @then ChecksError::kInvalidDeal occurs
   */
  TEST_F(CheckPieces, FillPieceNotEqualCID) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    info->pieces = {
        Piece{
            .piece = PieceInfo{.size = PaddedPieceSize(2048),
                               .cid = "010001020001"_cid},
            .deal_info = boost::none,
        },
    };

    api_->ChainHead = []() -> outcome::result<TipsetCPtr> {
      return outcome::success();
    };

    EXPECT_OUTCOME_ERROR(ChecksError::kInvalidDeal,
                         checkPieces(miner_addr_, info, api_));
  }

  /**
   * @given filler piece with correct CID
   * @when try to check pieces
   * @then success
   */
  TEST_F(CheckPieces, FillPieceEqualCID) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    EXPECT_OUTCOME_TRUE(
        cid,
        CID::fromString("baga6ea4seaqpy7usqklokfx2vxuynmupslkeutzexe2uqurdg5vht"
                        "ebhxqmpqmy"));  // from lotus
    info->pieces = {
        Piece{
            .piece = PieceInfo{.size = PaddedPieceSize(2048), .cid = cid},
            .deal_info = boost::none,
        },
    };

    api_->ChainHead = []() -> outcome::result<TipsetCPtr> {
      return outcome::success();
    };

    EXPECT_OUTCOME_TRUE_1(checkPieces(miner_addr_, info, api_));
  }

  /**
   * @given piece, api, deal
   * @when try to check pieces, but deal proposal has wrong provider
   * @then ChecksError::kInvalidDeal occurs
   */
  TEST_F(CheckPieces, WrongProvider) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;
    info->pieces = {
        Piece{
            .piece = PieceInfo{.size = PaddedPieceSize(2048),
                               .cid = "010001020001"_cid},
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    TipsetKey head_key;
    api_->ChainHead = [&head_key]() -> outcome::result<TipsetCPtr> {
      auto tip =
          std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
      return tip;
    };

    api_->StateMarketStorageDeal =
        [id{miner_id_}, deal_id, &head_key](
            api::DealId did,
            const TipsetKey &key) -> outcome::result<api::StorageDeal> {
      if (did == deal_id and key == head_key) {
        api::StorageDeal res;
        res.proposal.provider = Address::makeFromId(id + 1);
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(ChecksError::kInvalidDeal,
                         checkPieces(miner_addr_, info, api_));
  }

  /**
   * @given piece, api, deal
   * @when try to check pieces, but deal proposal has wrong piece cid
   * @then ChecksError::kInvalidDeal occurs
   */
  TEST_F(CheckPieces, WrongPieceCid) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;
    info->pieces = {
        Piece{
            .piece = PieceInfo{.size = PaddedPieceSize(2048),
                               .cid = "010001020001"_cid},
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    TipsetKey head_key;
    api_->ChainHead = [&head_key]() -> outcome::result<TipsetCPtr> {
      auto tip =
          std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
      return tip;
    };

    api_->StateMarketStorageDeal =
        [id{miner_id_}, deal_id, &head_key](
            api::DealId did,
            const TipsetKey &key) -> outcome::result<api::StorageDeal> {
      if (did == deal_id and key == head_key) {
        api::StorageDeal res;
        res.proposal.provider = Address::makeFromId(id);
        res.proposal.piece_cid = "010001020002"_cid;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(ChecksError::kInvalidDeal,
                         checkPieces(miner_addr_, info, api_));
  }

  /**
   * @given piece, api, deal
   * @when try to check pieces, but deal proposal has wrong piece size
   * @then ChecksError::kInvalidDeal occurs
   */
  TEST_F(CheckPieces, WrongPieceSize) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;

    PieceInfo piece{.size = PaddedPieceSize(2048), .cid = "010001020001"_cid};
    info->pieces = {
        Piece{
            .piece = piece,
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    TipsetKey head_key;
    api_->ChainHead = [&head_key]() -> outcome::result<TipsetCPtr> {
      auto tip =
          std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
      return tip;
    };

    api_->StateMarketStorageDeal =
        [&piece, id{miner_id_}, deal_id, &head_key](
            api::DealId did,
            const TipsetKey &key) -> outcome::result<api::StorageDeal> {
      if (did == deal_id and key == head_key) {
        api::StorageDeal res;
        res.proposal.provider = Address::makeFromId(id);
        res.proposal.piece_cid = piece.cid;
        res.proposal.piece_size = piece.size + 1;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(ChecksError::kInvalidDeal,
                         checkPieces(miner_addr_, info, api_));
  }

  /**
   * @given piece, api, deal
   * @when try to check pieces, but deal proposal has expired start epoch
   * @then ChecksError::kExpiredDeal occurs
   */
  TEST_F(CheckPieces, ExpiredDeal) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;

    PieceInfo piece{.size = PaddedPieceSize(2048), .cid = "010001020001"_cid};
    info->pieces = {
        Piece{
            .piece = piece,
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    TipsetKey head_key;
    api_->ChainHead = [&head_key]() -> outcome::result<TipsetCPtr> {
      auto tip =
          std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
      return tip;
    };

    api_->StateMarketStorageDeal =
        [&piece, id{miner_id_}, deal_id, &head_key](
            api::DealId did,
            const TipsetKey &key) -> outcome::result<api::StorageDeal> {
      if (did == deal_id and key == head_key) {
        api::StorageDeal res;
        res.proposal.provider = Address::makeFromId(id);
        res.proposal.piece_cid = piece.cid;
        res.proposal.piece_size = piece.size;
        res.proposal.start_epoch = 0;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(ChecksError::kExpiredDeal,
                         checkPieces(miner_addr_, info, api_));
  }

}  // namespace fc::mining::checks
