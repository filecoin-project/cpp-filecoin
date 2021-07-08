/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "miner/storage_fsm/impl/precommit_batcher_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/libp2p/scheduler_mock.hpp"

namespace fc::mining {
  using api::MinerInfo;
  using api::SignedMessage;
  using api::UnsignedMessage;
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
      block.height = 9;

      auto tipset = std::make_shared<Tipset>(
          TipsetKey(), std::vector<api::BlockHeader>({block}));

      api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> {
        return outcome::success(tipset);
      };

      api_->StateMinerInfo =
          [&](const Address &address,
              const TipsetKey &tipset_key) -> outcome::result<MinerInfo> {
        if (address == miner_address_) {
          MinerInfo info;
          info.seal_proof_type = seal_proof_type_;
          return info;
        }
      };
      EXPECT_CALL(*sch_, now()).WillOnce(testing::Return(current_time_));
      auto result = PreCommitBatcherImpl::makeBatcher(
          toTicks(std::chrono::seconds(10)), api_, sch_, miner_address_);
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
  };

  TEST_F(PreCommitBatcherTest, BatcherSetup) {}

  TEST_F(PreCommitBatcherTest, BatcherWrite) {
    api::BlockHeader block;
    block.height = 9;
    auto tipset = std::make_shared<Tipset>(
        TipsetKey(), std::vector<api::BlockHeader>({block}));
    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> {
      return outcome::success(tipset);
    };

    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    primitives::TokenAmount deposit = 10;
    ASSERT_FALSE(batcher_->addPreCommit(si, deposit, precInf).has_error());
  };

  TEST_F(PreCommitBatcherTest, CallbackSend) {
    api::BlockHeader block;
    block.height = 9;
    primitives::TokenAmount mutualDeposit = 0;
    bool isCalled = false;
    auto tipset = std::make_shared<Tipset>(
        TipsetKey(), std::vector<api::BlockHeader>({block}));

    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> {
      return outcome::success(tipset);
    };

    api_->MpoolPushMessage = [&isCalled, &mutualDeposit](
                                 const UnsignedMessage &msg,
                                 const boost::optional<api::MessageSendSpec> &)
        -> outcome::result<SignedMessage> {
      std::cout << "testing\n";
      if (msg.method == vm::actor::builtin::v0::miner::PreCommitSector::Number
          && msg.value == mutualDeposit) {
        isCalled = true;
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
            testing::Return(current_time_ + toTicks(std::chrono::seconds(11))))
        .WillOnce(
            testing::Return(current_time_ + toTicks(std::chrono::seconds(11))))
        .WillRepeatedly(
            testing::Return(current_time_ + toTicks(std::chrono::seconds(11))));
    sch_->next_clock();
    ASSERT_TRUE(isCalled);
  }

}  // namespace fc::mining
