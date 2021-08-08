/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "markets/storage/chain_events/impl/chain_events_impl.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

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
  using vm::actor::MethodParams;
  using vm::actor::builtin::v0::miner::PreCommitSector;
  using vm::actor::builtin::v0::miner::ProveCommitSector;
  using vm::actor::builtin::v0::miner::SectorPreCommitInfo;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  class ChainEventsTest : public ::testing::Test {
   public:
    Address provider = Address::makeFromId(1);
    DealId deal_id{1};
    SectorNumber sector_number{13};
    std::shared_ptr<FullNodeApi> api{std::make_shared<FullNodeApi>()};
    std::shared_ptr<ChainEventsImpl> events{
        std::make_shared<ChainEventsImpl>(api)};
  };

  /**
   * @given subscription to events by address and deal id
   * @when PreCommit and then ProveCommit called
   * @then event is triggered
   */
  TEST_F(ChainEventsTest, CommitSector) {
    const auto block_cid{CbCid::hash("01"_unhex)};

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

      if (asBlake(cid) != block_cid) throw "wrong block requested";
      return BlockMessages{.bls = {pre_commit_message, prove_commit_message}};
    }};

    api->ChainNotify = {
        [block_cid]() -> outcome::result<Chan<std::vector<HeadChange>>> {
          auto channel{std::make_shared<Channel<std::vector<HeadChange>>>()};

          auto tipset = std::make_shared<Tipset>();
          tipset->key = TipsetKey{{block_cid}};
          HeadChange change{.type = HeadChangeType::APPLY, .value = tipset};
          channel->write({change});

          return Chan{std::move(channel)};
        }};

    api->StateWaitMsg = [](auto &, auto, auto, auto) {
      auto wait{api::Wait<api::MsgWait>::make()};
      wait.channel->write(outcome::success());
      return wait;
    };

    bool is_called = false;
    events->onDealSectorCommitted(
        provider, deal_id, [&]() { is_called = true; });

    EXPECT_OUTCOME_TRUE_1(events->init());

    EXPECT_TRUE(is_called);
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
    bool is_called = false;
    events->onDealSectorCommitted(
        provider, deal_id, [&]() { is_called = true; });
    EXPECT_FALSE(is_called);
  }

}  // namespace fc::markets::storage::chain_events
