/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/basic/scheduler/manual_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <miner/storage_fsm/types.hpp>

#include "miner/storage_fsm/impl/precommit_batcher_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/v5/miner/miner_actor.hpp"

namespace fc::mining {
  using api::MinerInfo;
  using api::SignedMessage;
  using api::UnsignedMessage;
  using fc::mining::types::DealInfo;
  using fc::mining::types::PaddedPieceSize;
  using fc::mining::types::Piece;
  using fc::mining::types::PieceInfo;
  using fc::mining::types::FeeConfig;
  using libp2p::basic::ManualSchedulerBackend;
  using libp2p::basic::Scheduler;
  using libp2p::basic::SchedulerImpl;
  using primitives::sector::RegisteredSealProof;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using vm::actor::builtin::v5::miner::PreCommitBatch;

  class PreCommitBatcherTest : public testing::Test {
   protected:
    virtual void SetUp() {
      mutual_deposit_ = 0;
      seal_proof_type_ = RegisteredSealProof::kStackedDrg2KiBV1;
      api_ = std::make_shared<FullNodeApi>();
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

      api_->ChainHead = [=]() -> outcome::result<TipsetCPtr> {
        return tipset_;
      };

      api_->StateMinerInfo =
          [=](const Address &address,
              const TipsetKey &tipset_key) -> outcome::result<MinerInfo> {
        if (address == miner_address_){
          MinerInfo info;
          info.seal_proof_type = seal_proof_type_;
          info.control = {wrong_side_address_, side_address_};
          return info;
        }
        return ERROR_TEXT("Error");
      };
      api_->WalletBalance = [=](const Address &address)->outcome::result<TokenAmount>{
        if(address == wrong_side_address_){
          return TokenAmount (5*10e19);
        }
        return TokenAmount(10e19);
      };

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

      std::shared_ptr<FeeConfig> fee_config = std::make_shared<FeeConfig>();
      fee_config->max_precommit_batch_gas_fee.base = static_cast<TokenAmount>(
          0.05
          * 10e18);  // TODO: config loading;
      fee_config->max_precommit_batch_gas_fee.per_sector =
          static_cast<TokenAmount>(0.25 * 10e18);
      MinerInfo miner_info{.control = {side_address_, miner_address_}};
      batcher_ = std::make_shared<PreCommitBatcherImpl>(
          std::chrono::seconds(60), api_, miner_address_, scheduler_, [=](MinerInfo miner_info,
                                                                             TokenAmount deposit,
                                                                             TokenAmount good_funds) -> outcome::result<Address> {
            Address minimal_balanced_address;
            auto finder = miner_info.control.end();
            for (auto address = miner_info.control.begin();
                 address != miner_info.control.end();
                 ++address) {
              if (api_->WalletBalance(*address).value() >= good_funds
                  && (finder == miner_info.control.end()
                      || api_->WalletBalance(*finder).value()
                         > api_->WalletBalance(*address).value())) {
                finder = address;
              }
            }
            if (finder == miner_info.control.end()) {
              return miner_info.worker;
            }
            if(*finder == side_address_){
              side_address_called = true;
            }
            return *finder;
          },
          fee_config);
    }

    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<ManualSchedulerBackend> scheduler_backend_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<PreCommitBatcherImpl> batcher_;
    std::shared_ptr<Tipset> tipset_;
    Address miner_address_;
    Address side_address_;
    Address wrong_side_address_;
    uint64_t miner_id_;
    RegisteredSealProof seal_proof_type_;
    TokenAmount mutual_deposit_;
    bool is_called_;
    bool side_address_called;
  };

  TEST_F(PreCommitBatcherTest, BatcherWrite) {
    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    TokenAmount deposit = 10;
    EXPECT_OUTCOME_TRUE_1(batcher_->addPreCommit(
        si, deposit, precInf, [](const outcome::result<CID> &cid)->outcome::result<void>{
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
    mutual_deposit_ = 0;
    is_called_ = false;
    side_address_called = false;

    SectorInfo si = SectorInfo();
    api::SectorPreCommitInfo precInf;
    TokenAmount deposit = 10;

    precInf.sealed_cid = "010001020005"_cid;
    si.sector_number = 2;

    EXPECT_OUTCOME_TRUE_1(batcher_->addPreCommit(
        si, deposit, precInf, [](const outcome::result<CID> &cid)->outcome::result<void>{
          EXPECT_OUTCOME_TRUE_1(cid);
          EXPECT_TRUE(cid.has_value());
          return outcome::success();
        }));
    mutual_deposit_ += 10;

    precInf.sealed_cid = "010001020006"_cid;
    si.sector_number = 3;

    EXPECT_OUTCOME_TRUE_1(batcher_->addPreCommit(
        si, deposit, precInf, [](const outcome::result<CID> &cid)->outcome::result<void> {
          EXPECT_TRUE(cid.has_value());
          return outcome::success();
        }));
    mutual_deposit_ += 10;

    scheduler_backend_->shiftToTimer();
    ASSERT_TRUE(is_called_);

    is_called_ = false;
    mutual_deposit_ = 0;

    precInf.sealed_cid = "010001020008"_cid;
    si.sector_number = 6;

    EXPECT_OUTCOME_TRUE_1(batcher_->addPreCommit(
        si, deposit, precInf, [](const outcome::result<CID> &cid)->outcome::result<void> {
        EXPECT_TRUE(cid.has_value());
          return outcome::success();
        }));
    mutual_deposit_ += 10;
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
    mutual_deposit_ = 0;
    is_called_ = false;
    side_address_called = false;

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

    EXPECT_OUTCOME_TRUE_1(batcher_->addPreCommit(
        si, deposit, precInf, [](const outcome::result<CID> &cid)->outcome::result<void> {
          EXPECT_TRUE(cid.has_value());
          return outcome::success();
        }));
    mutual_deposit_ += 10;

    scheduler_backend_->shiftToTimer();
    EXPECT_TRUE(is_called_);

    is_called_ = false;
    si.pieces = {};
    mutual_deposit_ = 0;

    deal.deal_schedule.start_epoch = 1;
    Piece p3 = {.piece = PieceInfo{.size = PaddedPieceSize(128),
                                   .cid = "010001020010"_cid},
                .deal_info = deal};
    si.pieces.push_back(p3);

    precInf.sealed_cid = "010001020013"_cid;
    si.sector_number = 4;

    EXPECT_OUTCOME_TRUE_1(batcher_->addPreCommit(
        si, deposit, precInf, [](const outcome::result<CID> &cid)->outcome::result<void> {
          EXPECT_TRUE(cid.has_value());
          return outcome::success();
        }));
    ASSERT_TRUE(side_address_called);
    mutual_deposit_ += 10;
    ASSERT_TRUE(is_called_);
  }
}  // namespace fc::mining
