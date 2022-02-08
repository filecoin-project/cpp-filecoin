/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "markets/storage/chain_events/impl/chain_events_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/mocks/std_function.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/v5/miner/miner_actor.hpp"
#include "vm/actor/builtin/v7/miner/miner_actor.hpp"

namespace fc::markets::storage::chain_events {
  using adt::Channel;
  using api::BlockMessages;
  using api::Chan;
  using api::FullNodeApi;
  using primitives::block::BlockHeader;
  using primitives::block::MsgMeta;
  using primitives::tipset::HeadChange;
  using primitives::tipset::HeadChangeType;
  using primitives::tipset::Tipset;
  using testing::_;
  using vm::actor::MethodParams;
  using vm::actor::builtin::v5::miner::PreCommitSector;
  using vm::actor::builtin::v5::miner::ProveCommitSector;
  using vm::actor::builtin::v5::miner::SectorPreCommitInfo;
  using vm::actor::builtin::v7::miner::ProveReplicaUpdates;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::version::NetworkVersion;

  const outcome::result<void> void_success{outcome::success()};
  const auto cid0{"010001020001"_cid};

  class ChainEventsTest : public ::testing::Test {
   public:
    using MockCb = MockStdFunction<ChainEventsImpl::CommitCb>;

    std::shared_ptr<FullNodeApi> api{std::make_shared<FullNodeApi>()};
    MOCK_API(api, ChainGetBlockMessages);
    MOCK_API(api, ChainNotify);
    MOCK_API(api, StateMarketStorageDeal);
    MOCK_API_CB(api, StateWaitMsg);

    MockStdFunction<ChainEventsImpl::IsDealPrecommited> is_deal_precommited;
    boost::asio::io_context io;

    Address provider = Address::makeFromId(1);
    DealId deal_id{1};
    SectorNumber sector_number{13};
    std::shared_ptr<ChainEventsImpl> events{std::make_shared<ChainEventsImpl>(
        api, is_deal_precommited.AsStdFunction())};
    Chan<std::vector<HeadChange>> head_chan{
        std::make_shared<Channel<std::vector<HeadChange>>>()};
    CbCid block0{CbCid::hash("00"_unhex)};
    CbCid block1{CbCid::hash("01"_unhex)};
    CbCid block2{CbCid::hash("02"_unhex)};

    void chainNotify(HeadChangeType type, CbCid block) {
      const auto ts{std::make_shared<Tipset>()};
      ts->key = {{block}};
      head_chan.channel->write({{type, ts}});
    }

    void SetUp() override {
      EXPECT_CALL(mock_ChainNotify, Call())
          .WillOnce(testing::Return(head_chan));
      EXPECT_OUTCOME_TRUE_1(events->init());
      chainNotify(HeadChangeType::CURRENT, block0);
    }

    void ioRunOne() {
      io.restart();
      io.run_one();
    }
  };

  /**
   * @given subscription to events by address and deal id
   * @when PreCommit and then ProveCommit called
   * @then event is triggered
   */
  TEST_F(ChainEventsTest, CommitSector) {
    const auto io_StateWaitMsg{
        testing::Invoke([&](auto cb, auto, auto, auto, auto) {
          io.post([cb{std::move(cb)}] { cb(outcome::success()); });
        })};

    EXPECT_CALL(mock_StateMarketStorageDeal, Call(_, _))
        .WillOnce(testing::Return(api::StorageDeal{}));
    EXPECT_CALL(is_deal_precommited,
                Call(TipsetKey{{block0}}, provider, deal_id))
        .WillOnce(testing::Return(boost::none));
    MockCb cb;
    events->onDealSectorCommitted(provider, deal_id, cb.AsStdFunction());

    SectorPreCommitInfo pre_commit_info;
    pre_commit_info.sealed_cid = cid0;
    pre_commit_info.deal_ids.emplace_back(deal_id);
    pre_commit_info.sector = sector_number;
    UnsignedMessage pre_commit_message;
    pre_commit_message.to = provider;
    pre_commit_message.method = PreCommitSector::Number;
    pre_commit_message.params = codec::cbor::encode(pre_commit_info).value();
    EXPECT_CALL(mock_ChainGetBlockMessages, Call(CID{block1}))
        .WillOnce(testing::Return(BlockMessages{{pre_commit_message}, {}, {}}));
    EXPECT_CALL(mock_StateWaitMsg, Call(_, _, _, _, _))
        .WillOnce(io_StateWaitMsg);
    chainNotify(HeadChangeType::APPLY, block1);
    ioRunOne();

    UnsignedMessage prove_commit_message;
    prove_commit_message.to = provider;
    prove_commit_message.method = ProveCommitSector::Number;
    prove_commit_message.params =
        codec::cbor::encode(ProveCommitSector::Params{sector_number, {}})
            .value();
    EXPECT_CALL(mock_ChainGetBlockMessages, Call(CID{block2}))
        .WillOnce(
            testing::Return(BlockMessages{{prove_commit_message}, {}, {}}));
    EXPECT_CALL(mock_StateWaitMsg, Call(_, _, _, _, _))
        .WillOnce(io_StateWaitMsg);
    chainNotify(HeadChangeType::APPLY, block2);
    EXPECT_CALL(cb, Call(void_success)).WillOnce(testing::Return());
    ioRunOne();
  }

  /**
   * @given call onDealSectorCommitted
   * @when no message committed
   * @then future in wait status
   */
  TEST_F(ChainEventsTest, WaitCommitSector) {
    api::StorageDeal deal;
    deal.state.sector_start_epoch = 1;
    EXPECT_CALL(mock_StateMarketStorageDeal, Call(_, _))
        .WillOnce(testing::Return(deal));

    MockCb cb;
    EXPECT_CALL(cb, Call(void_success)).WillOnce(testing::Return());
    events->onDealSectorCommitted(provider, deal_id, cb.AsStdFunction());
  }

  TEST_F(ChainEventsTest, Update) {
    EXPECT_CALL(mock_StateMarketStorageDeal, Call(_, _))
        .WillOnce(testing::Return(api::StorageDeal{}));
    EXPECT_CALL(is_deal_precommited,
                Call(TipsetKey{{block0}}, provider, deal_id))
        .WillOnce(testing::Return(boost::none));
    MockCb cb;
    events->onDealSectorCommitted(provider, deal_id, cb.AsStdFunction());

    ProveReplicaUpdates::Params params;
    auto &update{params.updates.emplace_back()};
    update.deals.emplace_back(deal_id);
    update.comm_r = CID{CbCid{}};
    UnsignedMessage msg;
    msg.to = provider;
    msg.method = ProveReplicaUpdates::Number;
    msg.params = codec::cbor::encode(params).value();
    EXPECT_CALL(mock_ChainGetBlockMessages, Call(CID{block1}))
        .WillOnce(testing::Return(BlockMessages{{msg}, {}, {}}));
    api::MsgWait wait;
    wait.receipt.return_value =
        codec::cbor::encode(ProveReplicaUpdates::Result{update.sector}).value();
    EXPECT_CALL(mock_StateWaitMsg, Call(_, _, _, _, _))
        .WillOnce([&](auto cb, auto, auto, auto, auto) {
          io.post([cb{std::move(cb)}, wait] { cb(wait); });
        });
    chainNotify(HeadChangeType::APPLY, block1);
    EXPECT_CALL(cb, Call(void_success)).WillOnce(testing::Return());
    ioRunOne();
  }
}  // namespace fc::markets::storage::chain_events
