/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"

#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/states/market/market_actor_state.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::testutil::vm::actor::builtin::market {
  using fc::vm::actor::kSendMethodNumber;
  using fc::vm::actor::builtin::states::MarketActorState;
  using fc::vm::actor::builtin::types::market::DealProposal;
  using fc::vm::state::StateTreeImpl;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using testing::_;
  using testing::Return;

  const auto some_cid = "01000102ffff"_cid;
  DealId deal_1_id = 13;
  DealId deal_2_id = 24;

  class MarketActorTestFixture : public ActorTestFixture<MarketActorState> {
   public:
    using ActorTestFixture<MarketActorState>::runtime;
    using ActorTestFixture<MarketActorState>::ipld;
    using ActorTestFixture<MarketActorState>::callerIs;
    using ActorTestFixture<MarketActorState>::currentEpochIs;
    using ActorTestFixture<MarketActorState>::current_epoch;
    using ActorTestFixture<MarketActorState>::state;

    void SetUp() override {
      ActorTestFixture<MarketActorState>::SetUp();
      runtime.resolveAddressWith(state_tree);
      currentEpochIs(50000);
    }

    void expectSendFunds(const Address &address, TokenAmount amount) {
      EXPECT_CALL(runtime, send(address, kSendMethodNumber, testing::_, amount))
          .WillOnce(Return(outcome::success()));
    }

    void expectHasDeal(DealId deal_id, const DealProposal &deal, bool has) {
      if (has) {
        EXPECT_OUTCOME_EQ(state->proposals.get(deal_id), deal);
      } else {
        EXPECT_OUTCOME_EQ(state->proposals.has(deal_id), has);
      }
    }

    DealProposal setupVerifyDealsOnSectorProveCommit(
        const std::function<void(DealProposal &)> &prepare) {
      DealProposal deal;
      deal.piece_size = 3;
      deal.piece_cid = some_cid;
      deal.provider = miner_address;
      deal.start_epoch = current_epoch;
      deal.end_epoch = deal.start_epoch + 10;
      prepare(deal);
      EXPECT_OUTCOME_TRUE_1(state->proposals.set(deal_1_id, deal));

      callerIs(miner_address);

      return deal;
    }

    Address miner_address{Address::makeFromId(100)};
    Address owner_address{Address::makeFromId(101)};
    Address worker_address{Address::makeFromId(102)};
    Address client_address{Address::makeFromId(103)};

    StateTreeImpl state_tree{ipld};
  };
}  // namespace fc::testutil::vm::actor::builtin::market
