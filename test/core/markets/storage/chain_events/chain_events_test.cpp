/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "markets/storage/chain_events/impl/chain_events_impl.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"

namespace fc::markets::storage::chain_events {
  using adt::Channel;
  using api::Api;
  using api::BlockMessages;
  using api::Chan;
  using fc::storage::ipfs::InMemoryDatastore;
  using fc::storage::ipfs::IpfsDatastore;
  using primitives::block::BlockHeader;
  using primitives::block::MsgMeta;
  using primitives::tipset::HeadChange;
  using primitives::tipset::HeadChangeType;
  using primitives::tipset::Tipset;
  using vm::actor::MethodParams;
  using vm::actor::builtin::miner::PreCommitSector;
  using vm::actor::builtin::miner::ProveCommitSector;
  using vm::actor::builtin::miner::SectorPreCommitInfo;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  class ChainEventsTest : public ::testing::Test {
   public:
    Address provider = Address::makeFromId(1);
    DealId deal_id{1};
    SectorNumber sector_number{13};
    std::shared_ptr<Api> api{std::make_shared<Api>()};
    std::shared_ptr<ChainEventsImpl> events{
        std::make_shared<ChainEventsImpl>(api)};
  };

  /**
   * @given subscription to events by address and deal id
   * @when PreCommit and then ProveCommit called
   * @then event is triggered
   */
  TEST_F(ChainEventsTest, CommitSector) {
    CID block_cid{"010001020002"_cid};

    api->ChainGetBlockMessages = {[block_cid, this](const CID &cid)
                                      -> outcome::result<BlockMessages> {
      // PreCommitSector message call
      SectorPreCommitInfo pre_commit_info;
      pre_commit_info.sealed_cid = "010001020001"_cid;
      pre_commit_info.deal_ids.emplace_back(deal_id);
      pre_commit_info.sector = sector_number;
      EXPECT_OUTCOME_TRUE(pre_commit_params,
                          codec::cbor::encode(pre_commit_info));
      UnsignedMessage pre_commit_message;
      pre_commit_message.to = provider;
      pre_commit_message.method = PreCommitSector::Number;
      pre_commit_message.params = MethodParams{pre_commit_params};

      // ProveCommitSector message call
      ProveCommitSector::Params prove_commit_param;
      prove_commit_param.sector = sector_number;
      EXPECT_OUTCOME_TRUE(encoded_prove_commit_params,
                          codec::cbor::encode(prove_commit_param));
      UnsignedMessage prove_commit_message;
      prove_commit_message.to = provider;
      prove_commit_message.method = ProveCommitSector::Number;
      prove_commit_message.params = MethodParams{encoded_prove_commit_params};

      if (cid != block_cid) throw "wrong block requested";
      return BlockMessages{.bls = {pre_commit_message, prove_commit_message}};
    }};

    api->ChainNotify = {
        [block_cid]() -> outcome::result<Chan<std::vector<HeadChange>>> {
          auto channel{std::make_shared<Channel<std::vector<HeadChange>>>()};

          Tipset tipset{.cids = {block_cid}};
          HeadChange change{.type = HeadChangeType::APPLY, .value = tipset};
          channel->write({change});

          return Chan{std::move(channel)};
        }};

    auto res = events->onDealSectorCommitted(provider, deal_id);

    EXPECT_OUTCOME_TRUE_1(events->init());

    auto future = res->get_future();
    EXPECT_EQ(std::future_status::ready,
              future.wait_for(std::chrono::seconds(0)));
    EXPECT_OUTCOME_TRUE_1(future.get());
  }

  /**
   * @given call onDealSectorCommitted
   * @when no message committed
   * @then future in wait status
   */
  TEST_F(ChainEventsTest, WaitCommitSector) {
    api->ChainNotify = {[]() -> outcome::result<Chan<std::vector<HeadChange>>> {
      auto channel{std::make_shared<Channel<std::vector<HeadChange>>>()};
      return Chan{std::move(channel)};
    }};

    EXPECT_OUTCOME_TRUE_1(events->init());
    auto res = events->onDealSectorCommitted(provider, deal_id);
    auto future = res->get_future();
    EXPECT_EQ(std::future_status::timeout,
              future.wait_for(std::chrono::seconds(0)));
  }

}  // namespace fc::markets::storage::chain_events
