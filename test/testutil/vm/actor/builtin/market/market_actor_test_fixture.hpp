/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"

#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/states/market_actor_state.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::testutil::vm::actor::builtin::market {
  using fc::vm::actor::kSendMethodNumber;
  using fc::vm::actor::builtin::types::market::DealProposal;
  using fc::vm::state::StateTreeImpl;
  using primitives::DealId;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using testing::_;
  using testing::Return;
  using BaseMarketActorState = fc::vm::actor::builtin::states::MarketActorState;

  const auto some_cid = "01000102ffff"_cid;
  DealId deal_1_id = 13;
  DealId deal_2_id = 24;

  template <class State>
  class MarketActorTestFixture : public ActorTestFixture<State> {
   public:
    using ActorTestFixture<State>::runtime;
    using ActorTestFixture<State>::ipld;
    using ActorTestFixture<State>::callerIs;
    using ActorTestFixture<State>::currentEpochIs;
    using ActorTestFixture<State>::current_epoch;
    using ActorTestFixture<State>::state_manager;
    using ActorTestFixture<State>::state;

    void SetUp() override {
      ActorTestFixture<State>::SetUp();

      runtime.resolveAddressWith(state_tree);

      currentEpochIs(50000);

      EXPECT_CALL(*state_manager, createMarketActorState(testing::_))
          .WillRepeatedly(testing::Invoke([&](auto) {
            auto s = std::make_shared<State>();
            loadState(*s);
            return std::static_pointer_cast<BaseMarketActorState>(s);
          }));

      EXPECT_CALL(*state_manager, getMarketActorState())
          .WillRepeatedly(testing::Invoke([&]() {
            EXPECT_OUTCOME_TRUE(cid, setCbor(ipld, state));
            EXPECT_OUTCOME_TRUE(current_state, getCbor<State>(ipld, cid));
            auto s = std::make_shared<State>(current_state);
            return std::static_pointer_cast<BaseMarketActorState>(s);
          }));
    }

    void loadState(State &s) {
      cbor_blake::cbLoadT(ipld, s);
    }

    void expectSendFunds(const Address &address, TokenAmount amount) {
      EXPECT_CALL(runtime, send(address, kSendMethodNumber, testing::_, amount))
          .WillOnce(Return(outcome::success()));
    }

    void expectHasDeal(DealId deal_id, const DealProposal &deal, bool has) {
      if (has) {
        EXPECT_OUTCOME_EQ(state.proposals.get(deal_id), deal);
      } else {
        EXPECT_OUTCOME_EQ(state.proposals.has(deal_id), has);
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
      EXPECT_OUTCOME_TRUE_1(state.proposals.set(deal_1_id, deal));

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
