/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "markets/storage/chain_events/impl/chain_events_impl.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v5/miner/miner_actor.hpp"

namespace fc::markets::storage::chain_events {
  using adt::Channel;
  using api::BlockMessages;
  using api::Chan;
  using api::FullNodeApi;
  using fc::storage::ipfs::InMemoryDatastore;
  using primitives::block::BlockHeader;
  using primitives::block::MsgMeta;
  using primitives::tipset::HeadChange;
  using primitives::tipset::HeadChangeType;
  using primitives::tipset::Tipset;
  using testing::_;
  using vm::actor::MethodParams;
  using vm::actor::builtin::v0::miner::PreCommitSector;
  using vm::actor::builtin::v0::miner::ProveCommitSector;
  using vm::actor::builtin::v0::miner::SectorPreCommitInfo;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;
  using vm::version::NetworkVersion;

  const outcome::result<void> void_success{outcome::success()};
  const auto cid0{"010001020001"_cid};

  class ChainEventsTest : public ::testing::Test {
   public:
    NetworkVersion network_version{NetworkVersion::kVersion13};
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    Address provider = Address::makeFromId(1);
    DealId deal_id{1};
    SectorNumber sector_number{13};
    std::shared_ptr<FullNodeApi> api{std::make_shared<FullNodeApi>()};
    std::shared_ptr<ChainEventsImpl> events{
        std::make_shared<ChainEventsImpl>(api)};
    Chan<std::vector<HeadChange>> head_chan{
        std::make_shared<Channel<std::vector<HeadChange>>>()};
    CbCid block0{CbCid::hash("00"_unhex)};
    CbCid block1{CbCid::hash("01"_unhex)};
    CbCid block2{CbCid::hash("02"_unhex)};

    MOCK_API(api, ChainGetBlockMessages);
    MOCK_API(api, ChainNotify);
    MOCK_API(api, StateGetActor);
    MOCK_API(api, StateMarketStorageDeal);
    MOCK_API(api, StateNetworkVersion);
    MOCK_API_CB(api, StateWaitMsg);

    using MockCb = testing::MockFunction<void(outcome::result<void>)>;

    void chainNotify(HeadChangeType type, CbCid block) {
      const auto ts{std::make_shared<Tipset>()};
      ts->key = {{block}};
      head_chan.channel->write({{type, ts}});
    }

    template <typename F>
    void withPrecommits(const CbCid &block, const F &f) {
      TipsetKey tsk{{block}};
      ipld->actor_version = actorVersion(network_version);
      auto state{vm::actor::builtin::states::makeEmptyMinerState(ipld).value()};
      state->miner_info.cid = cid0;
      state->vesting_funds.cid = cid0;
      f([&](SectorNumber sector, std::vector<DealId> deal_ids) {
        vm::actor::builtin::types::miner::SectorPreCommitOnChainInfo info;
        info.info.deal_ids = std::move(deal_ids);
        state->precommitted_sectors.set(sector, info).value();
      });
      vm::actor::Actor actor;
      actor.head = setCbor(ipld, state).value();
      EXPECT_CALL(mock_StateGetActor, Call(provider, tsk))
          .WillRepeatedly(testing::Return(actor));
      EXPECT_CALL(mock_StateNetworkVersion, Call(tsk))
          .WillRepeatedly(testing::Return(network_version));
    }

    void SetUp() override {
      api->ChainReadObj = [this](const CID &cid) { return ipld->get(cid); };
      EXPECT_CALL(mock_ChainNotify, Call())
          .WillOnce(testing::Return(head_chan));
      EXPECT_OUTCOME_TRUE_1(events->init());
      chainNotify(HeadChangeType::CURRENT, block0);
    }
  };

  /**
   * @given subscription to events by address and deal id
   * @when PreCommit and then ProveCommit called
   * @then event is triggered
   */
  TEST_F(ChainEventsTest, CommitSector) {
    boost::asio::io_context io;
    const auto io_StateWaitMsg{
        testing::Invoke([&](auto cb, auto, auto, auto, auto) {
          io.post([cb{std::move(cb)}] { cb(outcome::success()); });
        })};
    const auto io_run_one{[&] {
      io.restart();
      io.run_one();
    }};

    EXPECT_CALL(mock_StateMarketStorageDeal, Call(_, _))
        .WillOnce(testing::Return(api::StorageDeal{}));
    withPrecommits(block0, [](auto) {});
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
    io_run_one();

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
    io_run_one();
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

}  // namespace fc::markets::storage::chain_events
