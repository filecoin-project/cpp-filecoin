/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "host/context/impl/host_context_impl.hpp"
#include "markets/storage/events/impl/events_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"

namespace fc::markets::storage::events {
  using adt::Channel;
  using api::Api;
  using api::Chan;
  using fc::storage::mpool::MpoolUpdate;
  using host::HostContextImpl;
  using vm::actor::MethodParams;
  using vm::actor::builtin::miner::PreCommitSector;
  using vm::actor::builtin::miner::ProveCommitSector;
  using vm::actor::builtin::miner::SectorPreCommitInfo;
  using vm::message::SignedMessage;
  using vm::message::UnsignedMessage;

  class EventsTest : public ::testing::Test {
   public:
    void SetUp() override {
      api = std::make_shared<Api>();
      events = std::make_shared<EventsImpl>(api);
    }

    Address provider = Address::makeFromId(1);
    DealId deal_id{1};
    SectorNumber sector_number{13};
    std::shared_ptr<Api> api;
    std::shared_ptr<EventsImpl> events;
  };

  /**
   * @given subscription to events by address and deal id
   * @when PreCommit and then ProveCommit called
   * @then event is triggered
   */
  TEST_F(EventsTest, CommitSector) {
    api->MpoolSub = {[=]() -> outcome::result<Chan<MpoolUpdate>> {
      auto channel{std::make_shared<Channel<MpoolUpdate>>()};

      // PreCommitSector message call
      SectorPreCommitInfo pre_commit_info;
      pre_commit_info.sealed_cid = "010001020001"_cid;
      pre_commit_info.deal_ids.emplace_back(deal_id);
      pre_commit_info.sector = sector_number;
      OUTCOME_TRY(pre_commit_params, codec::cbor::encode(pre_commit_info));
      UnsignedMessage pre_commit_message;
      pre_commit_message.to = provider;
      pre_commit_message.method = PreCommitSector::Number;
      pre_commit_message.params = MethodParams{pre_commit_params};
      SignedMessage pre_commit_signed_message;
      pre_commit_signed_message.message = pre_commit_message;
      MpoolUpdate pre_commit_update{};
      pre_commit_update.type = MpoolUpdate::Type::REMOVE;
      pre_commit_update.message = pre_commit_signed_message;
      channel->write(pre_commit_update);

      // ProveCommitSector message call
      ProveCommitSector::Params prove_commit_param;
      prove_commit_param.sector = sector_number;
      OUTCOME_TRY(encoded_prove_commit_params,
                  codec::cbor::encode(prove_commit_param));
      UnsignedMessage prove_commit_message;
      prove_commit_message.to = provider;
      prove_commit_message.method = ProveCommitSector::Number;
      prove_commit_message.params = MethodParams{encoded_prove_commit_params};
      SignedMessage prove_commit_signed_message;
      prove_commit_signed_message.message = prove_commit_message;
      MpoolUpdate prove_commit_update{};
      prove_commit_update.type = MpoolUpdate::Type::REMOVE;
      prove_commit_update.message = prove_commit_signed_message;
      channel->write(prove_commit_update);

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
  TEST_F(EventsTest, WaitCommitSector) {
    api->MpoolSub = {[=]() -> outcome::result<Chan<MpoolUpdate>> {
      auto channel{std::make_shared<Channel<MpoolUpdate>>()};
      return Chan{std::move(channel)};
    }};

    EXPECT_OUTCOME_TRUE_1(events->init());
    auto res = events->onDealSectorCommitted(provider, deal_id);
    auto future = res->get_future();
    EXPECT_EQ(std::future_status::timeout,
              future.wait_for(std::chrono::seconds(0)));
  }

}  // namespace fc::markets::storage::events
