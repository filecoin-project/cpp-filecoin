/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <miner/storage_fsm/types.hpp>
#include "miner/storage_fsm/impl/precommit_batcher_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/libp2p/scheduler_mock.hpp"

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

  class PreCommitBatcherTest : public testing::Test {
   protected:
    virtual void SetUp() {
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;
      api_ = std::make_shared<FullNodeApi>();
      sch_ = std::make_shared<SchedulerMock>();
      miner_id = 42;
      miner_address_ = Address::makeFromId(miner_id);
      current_time_ = toTicks(std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch()));

      api::BlockHeader block;
      block.height = 2;

      auto tipset = std::make_shared<Tipset>(
          TipsetKey(), std::vector<api::BlockHeader>({block}));

      api_->ChainHead = [=]() -> outcome::result<TipsetCPtr> {
        return outcome::success(tipset);
      };

      api_->StateMinerInfo =
          [=](const Address &address,
              const TipsetKey &tipset_key) -> outcome::result<MinerInfo> {
        if (address == miner_address_) {
          MinerInfo info;
          info.seal_proof_type = seal_proof_type_;
          return info;
        }
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
    Address miner_address_;
    uint64_t miner_id;
    RegisteredSealProof seal_proof_type_;
    Ticks current_time_;
    primitives::TokenAmount mutualDeposit = 0;
  };

  /** BatcherWrite checking that tgeё

  TEST_F(PreCommitBatcherTest, BatcherWrite) {
    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    primitives::TokenAmount deposit = 10;
    ASSERT_FALSE(batcher_->addPreCommit(si, deposit, precInf).has_error());
  };

  /**
   * CallbackSend test is checking that after the scheduled time for a Precommit
   * collecting, all the stored batcher's data will be published in  a
   * messagePool
   */
  TEST_F(PreCommitBatcherTest, CallbackSend) {
    mutualDeposit = 0;
    bool isCall = false;

    api::BlockHeader block;
    block.height = 2;

    auto tipset = std::make_shared<Tipset>(
        TipsetKey(), std::vector<api::BlockHeader>({block}));

    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> {
      return outcome::success(tipset);
    };

    api_->MpoolPushMessage = [&](const UnsignedMessage &msg,
                                 const boost::optional<api::MessageSendSpec> &)
        -> outcome::result<SignedMessage> {
      std::cout << "testing Sending\n";
      if (msg.method == vm::actor::builtin::v0::miner::PreCommitSector::Number
          && msg.value == mutualDeposit) {
        isCall = true;
        return SignedMessage{.message = msg, .signature = BlsSignature()};
      }

      return ERROR_TEXT("ERROR");
    };

    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    primitives::TokenAmount deposit = 10;

    precInf.sealed_cid = "010001020005"_cid;
    si.sector_number = 2;

    auto maybe_result = batcher_->addPreCommit(si, deposit, precInf);
    ASSERT_FALSE(maybe_result.has_error());
    mutualDeposit += 10;

    precInf.sealed_cid = "010001020006"_cid;
    si.sector_number = 3;

    maybe_result = batcher_->addPreCommit(si, deposit, precInf);
    ASSERT_FALSE(maybe_result.has_error());
    mutualDeposit += 10;

    EXPECT_CALL(*sch_, now())
        .WillOnce(
            testing::Return(current_time_ + toTicks(std::chrono::seconds(61))))
        .WillRepeatedly(
            testing::Return(current_time_ + toTicks(std::chrono::seconds(61))));
    sch_->next_clock();
    ASSERT_TRUE(isCall);
  }

  /**
   * ShortDistanceSending checking cutoff functionality
   * that makes PreCommitBatcher rescheduling to be sure,
   * that PreCommits with a short scheduled deals will be published
   * in message pool before the deadline.
   **/
  TEST_F(PreCommitBatcherTest, ShortDistanceSending) {
    mutualDeposit = 0;
    bool isCall = false;
    EXPECT_CALL(*sch_, now()).WillOnce(testing::Return(current_time_));

    api::BlockHeader block;
    block.height = 2;

    auto tipset = std::make_shared<Tipset>(
        TipsetKey(), std::vector<api::BlockHeader>({block}));

    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> {
      return outcome::success(tipset);
    };

    api_->MpoolPushMessage = [&](const UnsignedMessage &msg,
                                 const boost::optional<api::MessageSendSpec> &)
        -> outcome::result<SignedMessage> {
      if (msg.method == vm::actor::builtin::v0::miner::PreCommitSector::Number
          && msg.value == mutualDeposit) {
        isCall = true;
        return SignedMessage{.message = msg, .signature = BlsSignature()};
      }

      return ERROR_TEXT("ERROR");
    };

    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    primitives::TokenAmount deposit = 10;
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

    auto maybe_result = batcher_->addPreCommit(si, deposit, precInf);
    ASSERT_FALSE(maybe_result.has_error());
    mutualDeposit += 10;

    EXPECT_CALL(*sch_, now())
        .WillOnce(testing::Return(
            current_time_
            + toTicks(std::chrono::seconds(kEpochDurationSeconds + 10))))
        .WillRepeatedly(
            testing::Return(current_time_ + toTicks(std::chrono::seconds(10))));
    sch_->next_clock();
    EXPECT_TRUE(isCall);
  }

}  // namespace fc::mining
