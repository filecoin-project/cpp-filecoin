/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/basic/scheduler/manual_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <miner/storage_fsm/types.hpp>

#include "miner/address_selector.hpp"
#include "miner/storage_fsm/impl/precommit_batcher_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/v5/miner/miner_actor.hpp"

namespace fc::mining {
  using api::MinerInfo;
  using api::SignedMessage;
  using api::UnsignedMessage;
  using fc::mining::types::DealInfo;
  using fc::mining::types::FeeConfig;
  using fc::mining::types::PaddedPieceSize;
  using fc::mining::types::Piece;
  using fc::mining::types::PieceInfo;
  using libp2p::basic::ManualSchedulerBackend;
  using libp2p::basic::Scheduler;
  using libp2p::basic::SchedulerImpl;
  using mining::SelectAddress;
  using primitives::sector::RegisteredSealProof;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using testing::_;
  using vm::actor::builtin::v5::miner::PreCommitBatch;
  using BatcherCallbackMock =
      std::function<void(const outcome::result<CID> &cid)>;

  class PreCommitBatcherTest : public testing::Test {
   protected:
    virtual void SetUp() {
      scheduler_backend_ = std::make_shared<ManualSchedulerBackend>();
      scheduler_ = std::make_shared<SchedulerImpl>(scheduler_backend_,
                                                   Scheduler::Config{});
      miner_id_ = 42;
      miner_address_ = Address::makeFromId(miner_id_);
      side_address_ = Address::makeFromId(++miner_id_);

      api::BlockHeader block;
      block.height = 2;

      tipset_ = std::make_shared<Tipset>(
          TipsetKey(), std::vector<api::BlockHeader>({block}));

      EXPECT_CALL(mock_ChainHead, Call()).WillRepeatedly(testing::Return(tipset_));

      MinerInfo minfo;
      minfo.window_post_proof_type =
          primitives::sector::RegisteredPoStProof::kStackedDRG2KiBWindowPoSt;
      minfo.control = {side_address_, wrong_side_address_};
      EXPECT_CALL(mock_StateMinerInfo, Call(miner_address_, _))
          .WillRepeatedly(testing::Return(minfo));
      EXPECT_CALL(mock_WalletBalance, Call(wrong_side_address_))
          .WillRepeatedly(
              testing::Return(TokenAmount("100000000000000000000")));

      api_->MpoolPushMessage =
          [&](const UnsignedMessage &msg,
              const boost::optional<api::MessageSendSpec> &)
          -> outcome::result<SignedMessage> {
        if (msg.method == PreCommitBatch::Number) {
          is_called_ = true;
          return SignedMessage{.message = msg, .signature = BlsSignature()};
        }

        return ERROR_TEXT("ERROR");
      };

      callback_mock_ = [](const outcome::result<CID> &cid) -> void {
        EXPECT_TRUE(cid.has_value());
      };

      std::shared_ptr<FeeConfig> fee_config = std::make_shared<FeeConfig>();
      fee_config->max_precommit_batch_gas_fee.base =
          TokenAmount{"50000000000000000"};
      fee_config->max_precommit_batch_gas_fee.per_sector =
          TokenAmount{"250000000000000"};
      MinerInfo miner_info{.control = {side_address_, miner_address_}};
      batcher_ = std::make_shared<PreCommitBatcherImpl>(
          std::chrono::seconds(60),
          api_,
          miner_address_,
          scheduler_,
          [=](const MinerInfo &miner_info,
              const TokenAmount &good_funds,
              const std::shared_ptr<FullNodeApi> &api)
              -> outcome::result<Address> {
            return SelectAddress(miner_info, good_funds, api);
          },
          fee_config);
    }

    std::shared_ptr<FullNodeApi> api_ = std::make_shared<FullNodeApi>();
    std::shared_ptr<ManualSchedulerBackend> scheduler_backend_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<PreCommitBatcherImpl> batcher_;
    std::shared_ptr<Tipset> tipset_;
    Address miner_address_;
    Address side_address_;
    Address wrong_side_address_;
    uint64_t miner_id_;
    BatcherCallbackMock callback_mock_;
    bool is_called_;
    MOCK_API(api_, StateMinerInfo);
    MOCK_API(api_, WalletBalance);
    MOCK_API(api_, ChainHead);
  };

  TEST_F(PreCommitBatcherTest, BatcherWrite) {
    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    TokenAmount deposit = 10;
    EXPECT_OUTCOME_TRUE_1(batcher_->addPreCommit(
        si,
        deposit,
        precInf,
        [](const outcome::result<CID> &cid) -> outcome::result<void> {
          return outcome::success();
        }));
  };

  /**
   * @given 4 precommits
   * @when send the first pair and after the rescheduling
   * next pair
   * @then The result should be 2 messages in message pool with pair of
   * precommits in each
   */
  TEST_F(PreCommitBatcherTest, CallbackSend) {
    is_called_ = false;

    EXPECT_CALL(mock_WalletBalance, Call(side_address_))
        .WillRepeatedly(testing::Return(TokenAmount("0")));
    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    TokenAmount deposit = 10;

    precInf.sealed_cid = "010001020005"_cid;
    si.sector_number = 2;

    EXPECT_OUTCOME_TRUE_1(
        batcher_->addPreCommit(si, deposit, precInf, callback_mock_));

    precInf.sealed_cid = "010001020006"_cid;
    si.sector_number = 3;

    EXPECT_OUTCOME_TRUE_1(
        batcher_->addPreCommit(si, deposit, precInf, callback_mock_));

    scheduler_backend_->shiftToTimer();
    ASSERT_TRUE(is_called_);

    is_called_ = false;

    precInf.sealed_cid = "010001020008"_cid;
    si.sector_number = 6;

    EXPECT_OUTCOME_TRUE_1(
        batcher_->addPreCommit(si, deposit, precInf, callback_mock_));

    scheduler_backend_->shiftToTimer();
    ASSERT_TRUE(is_called_);
    is_called_ = false;
  }

  /**
   * @given 3 precommits
   * @when first two are sent after 30 sec instead of 60 and one more
   * immediately after the addition.
   * @then The result should be 2 new messages in message pool 1st pair:
   * have been sent after 30  sec, and 2nd pair:
   * have been sent after the first one immediately
   **/
  TEST_F(PreCommitBatcherTest, ShortDistanceSending) {
    is_called_ = false;

    EXPECT_CALL(mock_WalletBalance, Call(side_address_))
        .WillOnce(testing::Return(TokenAmount("5000000000000000000000")))
        .WillRepeatedly(testing::Return(TokenAmount("0")));

    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    TokenAmount deposit = 10;
    si.ticket_epoch = 5;
    Piece p{.piece = PieceInfo{.size = PaddedPieceSize(128),
                               .cid = "010001020008"_cid},
            .deal_info = boost::none};
    si.pieces.push_back(p);
    DealInfo deal{};
    deal.deal_schedule.start_epoch = 3;
    Piece p2 = {.piece = PieceInfo{.size = PaddedPieceSize(128),
                                   .cid = "010001020009"_cid},
                .deal_info = deal};
    si.pieces.push_back(p2);

    precInf.sealed_cid = "010001020005"_cid;
    si.sector_number = 2;

    EXPECT_OUTCOME_TRUE_1(
        batcher_->addPreCommit(si, deposit, precInf, callback_mock_));
    scheduler_backend_->shiftToTimer();
    EXPECT_TRUE(is_called_);

    is_called_ = false;
    si.pieces = {};

    deal.deal_schedule.start_epoch = 1;
    Piece p3 = {.piece = PieceInfo{.size = PaddedPieceSize(128),
                                   .cid = "010001020010"_cid},
                .deal_info = deal};
    si.pieces.push_back(p3);

    precInf.sealed_cid = "010001020013"_cid;
    si.sector_number = 4;

    EXPECT_OUTCOME_TRUE_1(
        batcher_->addPreCommit(si, deposit, precInf, callback_mock_));
    ASSERT_TRUE(is_called_);

    is_called_ = false;

    EXPECT_OUTCOME_TRUE_1(
        batcher_->addPreCommit(si, deposit, precInf, callback_mock_));
    ASSERT_TRUE(is_called_);
  }
}  // namespace fc::mining
