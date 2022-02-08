/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/basic/scheduler/manual_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <miner/storage_fsm/types.hpp>
#include "miner/address_selector.hpp"
#include "miner/storage_fsm/impl/commit_batcher_impl.hpp"
#include "testutil/default_print.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/mocks/proofs/proof_engine_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v5/miner/miner_actor.hpp"
#include "vm/actor/builtin/v6/monies.hpp"
#include "vm/message/message.hpp"

namespace fc::mining {
  using primitives::ActorId;
  using testing::_;
  using vm::actor::builtin::v5::miner::ProveCommitAggregate;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using PairStorage = CommitBatcherImpl::PairStorage;
  using MapPairStorage = std::map<SectorNumber, PairStorage>;
  using fc::mining::types::FeeConfig;
  using fc::mining::types::PaddedPieceSize;
  using fc::mining::types::Piece;
  using fc::mining::types::PieceInfo;
  using libp2p::basic::ManualSchedulerBackend;
  using libp2p::basic::Scheduler;
  using libp2p::basic::SchedulerImpl;
  using mining::SelectAddress;
  using proofs::ProofEngineMock;
  using vm::actor::builtin::types::miner::SectorPreCommitOnChainInfo;
  using BatcherCallbackMock =
      std::function<void(const outcome::result<CID> &cid)>;

  MATCHER_P(methodMatcher, method, "compare msg method name") {
    return (arg.method == method);
  }

  class CommitBatcherTest : public testing::Test {
   protected:
    void SetUp() override {
      scheduler_backend_ = std::make_shared<ManualSchedulerBackend>();
      scheduler_ = std::make_shared<SchedulerImpl>(scheduler_backend_,
                                                   Scheduler::Config{});

      miner_id_ = 1;
      miner_address_ = Address::makeFromId(miner_id_);
      side_address_ = Address::makeFromId(++miner_id_);

      api::BlockHeader block;
      block.height = 2;

      tipset_ = std::make_shared<Tipset>(
          TipsetKey(), std::vector<api::BlockHeader>({block}));

      EXPECT_CALL(mock_ChainHead, Call())
          .WillRepeatedly(testing::Return(tipset_));

      fee_config_ = std::make_shared<FeeConfig>();
      fee_config_->max_precommit_batch_gas_fee.base =
          TokenAmount{"50000000000000000"};
      fee_config_->max_precommit_batch_gas_fee.per_sector =
          TokenAmount{"250000000000000"};

      EXPECT_CALL(mock_StateMinerInitialPledgeCollateral,
                  Call(miner_address_, _, _))
          .WillRepeatedly(testing::Return(TokenAmount(100)));

      EXPECT_CALL(mock_StateSectorPreCommitInfo, Call(miner_address_, _, _))
          .WillRepeatedly(testing::Return(SectorPreCommitOnChainInfo()));
      proof_ = std::make_shared<ProofEngineMock>();

      EXPECT_CALL(mock_StateMinerInfo, Call(_, _))
          .WillRepeatedly(testing::Return(MinerInfo()));

      EXPECT_CALL(mock_ChainGetTipSet, Call(_))
          .WillRepeatedly(testing::Return(tipset_));

      callback_mock_ = [](const outcome::result<CID> &cid) -> void {
        EXPECT_TRUE(cid.has_value());
      };
    }

    std::shared_ptr<FullNodeApi> api_ = std::make_shared<FullNodeApi>();
    std::shared_ptr<Scheduler> scheduler_;
    MapPairStorage pair_storage_;
    std::shared_ptr<Tipset> tipset_;
    Address miner_address_;
    Address side_address_;
    Address wrong_side_address_;
    ActorId miner_id_;
    std::shared_ptr<ProofEngineMock> proof_;
    std::shared_ptr<FeeConfig> fee_config_;
    BatcherCallbackMock callback_mock_;
    std::shared_ptr<ManualSchedulerBackend> scheduler_backend_;

    MOCK_API(api_, ChainHead);
    MOCK_API(api_, MpoolPushMessage);
    MOCK_API(api_, ChainGetTipSet);
    MOCK_API(api_, StateMinerInfo);
    MOCK_API(api_, StateMinerInitialPledgeCollateral);
    MOCK_API(api_, StateSectorPreCommitInfo);
  };

  /**
   * @given 2 commits and max_size_callback is 2
   * @when send the 2 commits
   * @then the result should be 2 messages in message pool with pair of commits
   */
  TEST_F(CommitBatcherTest, SendAfterMaxSize) {
    EXPECT_CALL(mock_MpoolPushMessage,
                Call(methodMatcher(ProveCommitAggregate::Number), _))
        .WillOnce(
            testing::Invoke([&](const UnsignedMessage &msg,
                                auto &) -> outcome::result<SignedMessage> {
              return SignedMessage{.message = msg, .signature = BlsSignature()};
            }));

    EXPECT_CALL(*proof_, aggregateSealProofs(_, _))
        .WillOnce(testing::Return(outcome::success()));

    std::shared_ptr<CommitBatcherImpl> batcher =
        std::make_shared<CommitBatcherImpl>(
            std::chrono::seconds(9999),
            api_,
            miner_address_,
            scheduler_,
            [=](const MinerInfo &miner_info,
                const TokenAmount &good_funds,
                const TokenAmount &need_funds,
                const std::shared_ptr<FullNodeApi> &api)
                -> outcome::result<Address> {
              return SelectAddress(miner_info, good_funds, api);
            },
            fee_config_,
            2,
            proof_);

    SectorInfo sector_info0 = SectorInfo();
    sector_info0.ticket_epoch = 5;
    Piece piece0{.piece = PieceInfo{.size = PaddedPieceSize(128),
                                    .cid = "010001020008"_cid},
                 .deal_info = boost::none};
    sector_info0.pieces.push_back(piece0);
    sector_info0.sector_number = 777;

    SectorInfo sector_info1 = SectorInfo();
    sector_info0.ticket_epoch = 5;
    Piece piece1{.piece = PieceInfo{.size = PaddedPieceSize(128),
                                    .cid = "010001020009"_cid},
                 .deal_info = boost::none};
    sector_info1.pieces.push_back(piece1);
    sector_info1.sector_number = 888;

    EXPECT_OUTCOME_TRUE_1(batcher->addCommit(
        sector_info0, AggregateInput{.info = 777}, callback_mock_));

    EXPECT_OUTCOME_TRUE_1(batcher->addCommit(
        sector_info1, AggregateInput{.info = 888}, callback_mock_));
  }

  /**
   * @given a 1 commit
   * @when send a 1 commit
   * @then result should be 0 messages in message pool
   */
  TEST_F(CommitBatcherTest, BatcherWrite) {
    std::shared_ptr<CommitBatcherImpl> batcher =
        std::make_shared<CommitBatcherImpl>(
            std::chrono::seconds(9999),
            api_,
            miner_address_,
            scheduler_,
            [=](const MinerInfo &miner_info,
                const TokenAmount &good_funds,
                const TokenAmount &need_funds,
                const std::shared_ptr<FullNodeApi> &api)
                -> outcome::result<Address> {
              return SelectAddress(miner_info, good_funds, api);
            },
            fee_config_,
            999,
            proof_);

    EXPECT_OUTCOME_TRUE_1(batcher->addCommit(
        SectorInfo(),
        AggregateInput(),
        [](const outcome::result<CID> &cid) -> outcome::result<void> {
          return outcome::success();
        }));
  }

  /**
   * @given 4 commits
   * @when send the first pair and after the rescheduling
   * next pair
   * @then The result should be 2 messages in message pool with pair of
   * commits in each
   */
  TEST_F(CommitBatcherTest, CallbackSend) {
    std::shared_ptr<CommitBatcherImpl> batcher =
        std::make_shared<CommitBatcherImpl>(
            std::chrono::seconds(999),
            api_,
            miner_address_,
            scheduler_,
            [=](const MinerInfo &miner_info,
                const TokenAmount &good_funds,
                const TokenAmount &need_funds,
                const std::shared_ptr<FullNodeApi> &api)
                -> outcome::result<Address> {
              return SelectAddress(miner_info, good_funds, api);
            },
            fee_config_,
            999,
            proof_);

    EXPECT_CALL(*proof_, aggregateSealProofs(_, _))
        .WillRepeatedly(testing::Return(outcome::success()));

    EXPECT_CALL(mock_MpoolPushMessage,
                Call(methodMatcher(ProveCommitAggregate::Number), _))
        .Times(2)
        .WillRepeatedly(
            testing::Invoke([&](const UnsignedMessage &msg,
                                auto &) -> outcome::result<SignedMessage> {
              return SignedMessage{.message = msg, .signature = BlsSignature()};
            }));

    SectorInfo sector_info = SectorInfo();

    sector_info.sector_number = 2;

    EXPECT_OUTCOME_TRUE_1(batcher->addCommit(
        sector_info, AggregateInput{.info = 2}, callback_mock_));

    scheduler_backend_->shiftToTimer();
    sector_info.sector_number = 3;

    EXPECT_OUTCOME_TRUE_1(batcher->addCommit(
        sector_info, AggregateInput{.info = 3}, callback_mock_));

    sector_info.sector_number = 6;

    EXPECT_OUTCOME_TRUE_1(batcher->addCommit(
        sector_info, AggregateInput{.info = 6}, callback_mock_));

    scheduler_backend_->shiftToTimer();
  }
}  // namespace fc::mining
