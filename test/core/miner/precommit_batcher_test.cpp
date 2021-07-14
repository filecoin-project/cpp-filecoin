/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <miner/storage_fsm/types.hpp>
#include "miner/storage_fsm/impl/precommit_batcher_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/libp2p/scheduler_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::mining {
  using api::MinerInfo;
  using api::SignedMessage;
  using api::UnsignedMessage;
  using fc::mining::types::DealInfo;
  using fc::mining::types::PaddedPieceSize;
  using fc::mining::types::Piece;
  using fc::mining::types::PieceInfo;
  using libp2p::protocol::SchedulerMock;
  using libp2p::protocol::scheduler::Ticks;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using testing::Mock;
  using libp2p::protocol::scheduler::toTicks;


  class PreCommitBatcherTest : public testing::Test {
   protected:
    virtual void SetUp() {
      mutualDeposit = 0;
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;
      api_ = std::make_shared<FullNodeApi>();
      sch_ = std::make_shared<SchedulerMock>();
      miner_id_ = 42;
      miner_address_ = Address::makeFromId(miner_id_);
      current_time_ = toTicks(std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch()));

      api::BlockHeader block;
      block.height = 2;

       tipset_ = std::make_shared<Tipset>(
          TipsetKey(), std::vector<api::BlockHeader>({block}));

      api_->ChainHead = [=]() -> outcome::result<TipsetCPtr> {
        return tipset_;
      };

      api_->StateMinerInfo =
          [=](const Address &address,
              const TipsetKey &tipset_key) -> outcome::result<MinerInfo> {
        if (address == miner_address_) {
          MinerInfo info;
          info.seal_proof_type = seal_proof_type_;
          return info;
        }
        return ERROR_TEXT("Error");
      };

      api_->MpoolPushMessage = [&](const UnsignedMessage &msg,
                                   const boost::optional<api::MessageSendSpec> &)
          -> outcome::result<SignedMessage> {
        if (msg.method == 25) {
          isCall = true;
          return SignedMessage{.message = msg, .signature = BlsSignature()};
        }

        return ERROR_TEXT("ERROR");
      };

      EXPECT_CALL(*sch_, now()).WillOnce(testing::Return(current_time_));
      auto result = PreCommitBatcherImpl::makeBatcher(
          toTicks(std::chrono::seconds(60)), api_, sch_, miner_address_);
      batcher_ = result.value();
      ASSERT_FALSE(result.has_error());
    }

    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<SchedulerMock> sch_;
    std::shared_ptr<PreCommitBatcherImpl> batcher_;
    std::shared_ptr<Tipset> tipset_;
    Address miner_address_;
    uint64_t miner_id_;
    RegisteredSealProof seal_proof_type_;
    Ticks current_time_;
    TokenAmount mutualDeposit;
    bool isCall;

  };

  TEST_F(PreCommitBatcherTest, BatcherWrite) {
    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    TokenAmount deposit = 10;
    ASSERT_FALSE(batcher_->addPreCommit(si, deposit, precInf).has_error());
  };

  /**
   * CallbackSend test is checking that after the scheduled time for a Precommit
   * collecting, all the stored batcher's data will be published in a
   * messagePool
   */
  TEST_F(PreCommitBatcherTest, CallbackSend) {
    mutualDeposit = 0;
    isCall = false;

    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    TokenAmount deposit = 10;

    precInf.sealed_cid = "010001020005"_cid;
    si.sector_number = 2;

    EXPECT_OUTCOME_TRUE_1(batcher_->addPreCommit(si, deposit, precInf));
    mutualDeposit += 10;

    precInf.sealed_cid = "010001020006"_cid;
    si.sector_number = 3;

    EXPECT_OUTCOME_TRUE_1(batcher_->addPreCommit(si, deposit, precInf));
    mutualDeposit += 10;

    EXPECT_CALL(*sch_, now())
        .WillOnce(
            testing::Return(current_time_ + toTicks(std::chrono::seconds(61))))
        .WillOnce(
            testing::Return(current_time_ + toTicks(std::chrono::seconds(123))))
        .WillRepeatedly(testing::Return(current_time_
                                        + toTicks(std::chrono::seconds(300))));
    sch_->next_clock();
    ASSERT_TRUE(isCall);

    isCall = false;
    mutualDeposit = 0;

    precInf.sealed_cid = "010001020008"_cid;
    si.sector_number = 6;

    EXPECT_OUTCOME_TRUE_1( batcher_->addPreCommit(si, deposit, precInf));
    mutualDeposit += 10;
    sch_->next_clock();
    ASSERT_TRUE(isCall);
    isCall = false;
  }

  /**
   * ShortDistanceSending checking cutoff functionality
   * that makes PreCommitBatcher rescheduling to be sure,
   * that PreCommits with a short scheduled deals will be published
   * in message pool before the deadline.
   **/
  TEST_F(PreCommitBatcherTest, ShortDistanceSending) {
    mutualDeposit = 0;
    isCall = false;

    EXPECT_CALL(*sch_, now())
        .WillOnce(testing::Return(current_time_))
        .WillOnce(testing::Return(
            current_time_ + toTicks(std::chrono::seconds(kEpochDurationSeconds))
            + 10))
        .WillRepeatedly(testing::Return(
            current_time_ + toTicks(std::chrono::seconds(kEpochDurationSeconds))
            + 12));

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

    EXPECT_OUTCOME_TRUE_1(batcher_->addPreCommit(si, deposit, precInf));
    mutualDeposit += 10;

    sch_->next_clock();
    EXPECT_TRUE(isCall);



    isCall = false;
    si.pieces = {};
    mutualDeposit = 0;

    deal.deal_schedule.start_epoch = 1;
    Piece p3 = {.piece = PieceInfo{.size = PaddedPieceSize(128),
                                   .cid = "010001020010"_cid},
                .deal_info = deal};
    si.pieces.push_back(p3);

    precInf.sealed_cid = "010001020013"_cid;
    si.sector_number = 4;

    EXPECT_OUTCOME_TRUE_1( batcher_->addPreCommit(si, deposit, precInf));
    mutualDeposit += 10;
    ASSERT_TRUE(isCall);
  }


}  // namespace fc::mining
