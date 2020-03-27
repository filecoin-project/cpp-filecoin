/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/market/actor.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/actor/builtin/market/policy.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

#define ON_CALL_3(a, b, c) \
  EXPECT_CALL(a, b).Times(testing::AnyNumber()).WillRepeatedly(Return(c))

namespace MarketActor = fc::vm::actor::builtin::market;
namespace MinerActor = fc::vm::actor::builtin::miner;
using fc::primitives::ChainEpoch;
using fc::primitives::DealId;
using fc::primitives::kChainEpochUndefined;
using fc::primitives::TokenAmount;
using fc::primitives::address::Address;
using fc::primitives::piece::PaddedPieceSize;
using fc::primitives::piece::PieceInfo;
using fc::primitives::sector::RegisteredProof;
using fc::storage::ipfs::InMemoryDatastore;
using fc::vm::VMExitCode;
using fc::vm::actor::ActorSubstateCID;
using fc::vm::actor::kAccountCodeCid;
using fc::vm::actor::kBurntFundsActorAddress;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::kInitCodeCid;
using fc::vm::actor::kSendMethodNumber;
using fc::vm::actor::kStorageMinerCodeCid;
using fc::vm::runtime::MockRuntime;
using fc::vm::state::StateTreeImpl;
using MarketActor::ClientDealProposal;
using MarketActor::DealProposal;
using MarketActor::DealState;
using MarketActor::State;
using testing::Return;

const auto some_cid = "01000102ffff"_cid;
DealId deal_1_id = 13, deal_2_id = 24;

/// State cbor encoding
TEST(MarketActorCborTest, State) {
  expectEncodeAndReencode(
      State{
          .proposals = decltype(State::proposals){"010001020001"_cid},
          .states = decltype(State::states){"010001020002"_cid},
          .escrow_table = decltype(State::escrow_table){"010001020003"_cid},
          .locked_table = decltype(State::locked_table){"010001020004"_cid},
          .next_deal = 1,
          .deals_by_party = decltype(State::deals_by_party){"010001020005"_cid},
      },
      "86d82a4700010001020001d82a4700010001020002d82a4700010001020003d82a470001000102000401d82a4700010001020005"_unhex);
}

/// DealState cbor encoding
TEST(MarketActorCborTest, DealState) {
  expectEncodeAndReencode(DealState{1, 2, 3}, "83010203"_unhex);
}

/// ClientDealProposal cbor encoding
TEST(MarketActorCborTest, ClientDealProposal) {
  expectEncodeAndReencode(
      ClientDealProposal{
          .proposal =
              DealProposal{
                  .piece_cid = "010001020001"_cid,
                  .piece_size = PaddedPieceSize{1},
                  .client = Address::makeFromId(1),
                  .provider = Address::makeFromId(2),
                  .start_epoch = 2,
                  .end_epoch = 3,
                  .storage_price_per_epoch = 4,
                  .provider_collateral = 5,
                  .client_collateral = 6,
              },
          .client_signature = "DEAD"_unhex,
      },
      "8289d82a47000100010200010142000142000202034200044200054200064301dead"_unhex);
}

struct MarketActorTest : testing::Test {
  void SetUp() override {
    ON_CALL_3(runtime, getIpfsDatastore(), ipld);

    EXPECT_CALL(runtime, resolveAddress(testing::_))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Invoke(
            [&](auto &address) { return state_tree.lookupId(address); }));

    ON_CALL_3(runtime, getCurrentEpoch(), epoch);

    ON_CALL_3(runtime, getActorCodeID(miner_address), kStorageMinerCodeCid);
    ON_CALL_3(runtime, getActorCodeID(owner_address), kAccountCodeCid);
    ON_CALL_3(runtime, getActorCodeID(worker_address), kAccountCodeCid);
    ON_CALL_3(runtime, getActorCodeID(client_address), kAccountCodeCid);
    ON_CALL_3(runtime, getActorCodeID(kInitAddress), kInitCodeCid);

    state.load(ipld);

    EXPECT_CALL(runtime, getCurrentActorState())
        .Times(testing::AtMost(1))
        .WillOnce(testing::Invoke([&]() {
          EXPECT_OUTCOME_TRUE_1(state.flush());
          EXPECT_OUTCOME_TRUE(cid, ipld->setCbor(state));
          return ActorSubstateCID{std::move(cid)};
        }));

    EXPECT_CALL(runtime, commit(testing::_))
        .Times(testing::AtMost(1))
        .WillOnce(testing::Invoke([&](auto &cid) {
          EXPECT_OUTCOME_TRUE(new_state,
                              ipld->getCbor<MarketActor::State>(cid));
          state = std::move(new_state);
          state.load(ipld);
          return fc::outcome::success();
        }));
  }

  void callerIs(const Address &caller) {
    ON_CALL_3(runtime, getImmediateCaller(), caller);
  }

  void expectSendFunds(const Address &address, TokenAmount amount) {
    EXPECT_CALL(runtime, send(address, kSendMethodNumber, testing::_, amount))
        .WillOnce(Return(fc::outcome::success()));
  }

  void expectPartyHasDeal(const Address &address, DealId deal, bool has) {
    EXPECT_OUTCOME_TRUE(set, state.deals_by_party.get(address));
    set.load(ipld);
    EXPECT_OUTCOME_EQ(set.has(deal), has);
  }

  void expectHasDeal(DealId deal_id, const DealProposal &deal, bool has) {
    if (has) {
      EXPECT_OUTCOME_EQ(state.proposals.get(deal_id), deal);
    } else {
      EXPECT_OUTCOME_EQ(state.proposals.has(deal_id), has);
    }
    for (auto address : {&deal.provider, &deal.client}) {
      EXPECT_OUTCOME_TRUE(set, state.deals_by_party.get(*address));
      set.load(ipld);
      EXPECT_OUTCOME_EQ(set.has(deal_id), has);
    }
  }

  DealProposal setupHandleExpiredDeals(
      const std::function<void(DealProposal &)> &prepare);

  ClientDealProposal setupPublishStorageDeals();

  DealProposal setupVerifyDealsOnSectorProveCommit(
      const std::function<void(DealProposal &)> &prepare);

  MockRuntime runtime;

  std::shared_ptr<InMemoryDatastore> ipld{
      std::make_shared<InMemoryDatastore>()};

  ChainEpoch epoch{2077};

  Address miner_address{Address::makeFromId(100)};
  Address owner_address{Address::makeFromId(101)};
  Address worker_address{Address::makeFromId(102)};
  Address client_address{Address::makeFromId(103)};

  MarketActor::State state;

  StateTreeImpl state_tree{ipld};
};

TEST_F(MarketActorTest, ConstructorCallerNotInit) {
  callerIs(client_address);

  EXPECT_OUTCOME_ERROR(VMExitCode::MARKET_ACTOR_WRONG_CALLER,
                       MarketActor::Construct::call(runtime, {}));
}

TEST_F(MarketActorTest, Constructor) {
  callerIs(kInitAddress);

  EXPECT_OUTCOME_TRUE_1(MarketActor::Construct::call(runtime, {}));
}

TEST_F(MarketActorTest, AddBalanceNominalNotSignable) {
  callerIs(client_address);

  EXPECT_OUTCOME_ERROR(VMExitCode::MARKET_ACTOR_WRONG_CALLER,
                       MarketActor::AddBalance::call(runtime, kInitAddress));
}

TEST_F(MarketActorTest, AddBalanceNominalNotOwnerOrWorker) {
  callerIs(kInitAddress);

  runtime.expectSendM<MinerActor::ControlAddresses>(
      miner_address, {}, 0, {owner_address, worker_address});

  EXPECT_OUTCOME_ERROR(VMExitCode::MARKET_ACTOR_WRONG_CALLER,
                       MarketActor::AddBalance::call(runtime, miner_address));
}

TEST_F(MarketActorTest, AddBalance) {
  TokenAmount amount{100};

  callerIs(miner_address);
  EXPECT_CALL(runtime, getValueReceived()).WillOnce(Return(amount));

  EXPECT_OUTCOME_TRUE_1(MarketActor::AddBalance::call(runtime, client_address));

  EXPECT_OUTCOME_EQ(state.escrow_table.get(client_address), amount);
  EXPECT_OUTCOME_EQ(state.locked_table.get(client_address), 0);
}

TEST_F(MarketActorTest, AddBalanceExisting) {
  TokenAmount escrow{210};
  TokenAmount locked{10};
  TokenAmount amount{100};

  EXPECT_OUTCOME_TRUE_1(state.escrow_table.set(client_address, escrow));
  EXPECT_OUTCOME_TRUE_1(state.locked_table.set(client_address, locked));

  callerIs(miner_address);
  EXPECT_CALL(runtime, getValueReceived()).WillOnce(Return(amount));

  EXPECT_OUTCOME_TRUE_1(MarketActor::AddBalance::call(runtime, client_address));

  EXPECT_OUTCOME_EQ(state.escrow_table.get(client_address), escrow + amount);
  EXPECT_OUTCOME_EQ(state.locked_table.get(client_address), locked);
}

TEST_F(MarketActorTest, WithdrawBalanceNegative) {
  callerIs(client_address);

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
      MarketActor::WithdrawBalance::call(runtime, {client_address, -1}));
}

TEST_F(MarketActorTest, WithdrawBalanceNominal) {
  TokenAmount escrow{100};
  TokenAmount locked{10};
  TokenAmount extracted{escrow - locked};

  EXPECT_OUTCOME_TRUE_1(state.escrow_table.set(client_address, escrow));
  EXPECT_OUTCOME_TRUE_1(state.locked_table.set(client_address, locked));

  callerIs(miner_address);
  expectSendFunds(client_address, extracted);

  EXPECT_OUTCOME_TRUE_1(
      MarketActor::WithdrawBalance::call(runtime, {client_address, escrow}));

  EXPECT_OUTCOME_EQ(state.escrow_table.get(client_address), escrow - extracted);
  EXPECT_OUTCOME_EQ(state.locked_table.get(client_address), locked);
}

TEST_F(MarketActorTest, WithdrawBalanceMiner) {
  TokenAmount escrow{100};
  TokenAmount locked{10};
  TokenAmount extracted{escrow - locked};

  EXPECT_OUTCOME_TRUE_1(state.escrow_table.set(miner_address, escrow));
  EXPECT_OUTCOME_TRUE_1(state.locked_table.set(miner_address, locked));

  callerIs(worker_address);
  runtime.expectSendM<MinerActor::ControlAddresses>(
      miner_address, {}, 0, {owner_address, worker_address});
  expectSendFunds(owner_address, extracted);

  EXPECT_OUTCOME_TRUE_1(
      MarketActor::WithdrawBalance::call(runtime, {miner_address, escrow}));

  EXPECT_OUTCOME_EQ(state.escrow_table.get(miner_address), escrow - extracted);
  EXPECT_OUTCOME_EQ(state.locked_table.get(miner_address), locked);
}

TEST_F(MarketActorTest, WithdrawBalanceUpdatePendingDeals) {
  DealProposal deal;
  deal.piece_cid = some_cid;
  EXPECT_OUTCOME_TRUE_1(state.addDeal(deal_1_id, deal));
  EXPECT_OUTCOME_TRUE_1(state.states.set(deal_1_id, {{}, epoch, {}}));
  EXPECT_OUTCOME_TRUE_1(state.escrow_table.set(client_address, 0));
  EXPECT_OUTCOME_TRUE_1(state.locked_table.set(client_address, 0));

  callerIs(miner_address);
  expectSendFunds(client_address, 0);

  EXPECT_OUTCOME_TRUE_1(
      MarketActor::WithdrawBalance::call(runtime, {client_address, 1}));

  EXPECT_OUTCOME_EQ(state.escrow_table.get(client_address), 0);
  EXPECT_OUTCOME_EQ(state.locked_table.get(client_address), 0);
}

DealProposal MarketActorTest::setupHandleExpiredDeals(
    const std::function<void(DealProposal &)> &prepare) {
  DealProposal deal;
  deal.piece_cid = some_cid;
  deal.start_epoch = epoch - 1;
  deal.end_epoch = epoch + 2;
  deal.provider = miner_address;
  deal.client = client_address;
  deal.storage_price_per_epoch = 1;
  deal.provider_collateral = 100;
  deal.client_collateral = 10;
  prepare(deal);
  EXPECT_OUTCOME_TRUE_1(state.addDeal(deal_1_id, deal));
  EXPECT_OUTCOME_TRUE_1(
      state.escrow_table.set(miner_address, deal.providerBalanceRequirement()));
  EXPECT_OUTCOME_TRUE_1(
      state.locked_table.set(miner_address, deal.providerBalanceRequirement()));
  EXPECT_OUTCOME_TRUE_1(
      state.escrow_table.set(client_address, deal.clientBalanceRequirement()));
  EXPECT_OUTCOME_TRUE_1(
      state.locked_table.set(client_address, deal.clientBalanceRequirement()));

  callerIs(worker_address);

  return deal;
}

TEST_F(MarketActorTest, HandleExpiredDealsCallerNotSignable) {
  callerIs(kInitAddress);

  EXPECT_OUTCOME_ERROR(VMExitCode::MARKET_ACTOR_WRONG_CALLER,
                       MarketActor::HandleExpiredDeals::call(runtime, {}));
}

TEST_F(MarketActorTest, HandleExpiredDealsAlreadyUpdated) {
  setupHandleExpiredDeals([](auto &) {});
  EXPECT_OUTCOME_TRUE_1(state.states.set(deal_1_id, {{}, epoch, {}}));

  expectSendFunds(kBurntFundsActorAddress, 0);

  EXPECT_OUTCOME_TRUE_1(
      MarketActor::HandleExpiredDeals::call(runtime, {{deal_1_id}}));
}

TEST_F(MarketActorTest, HandleExpiredDealsNotStarted) {
  setupHandleExpiredDeals([&](auto &deal) { deal.start_epoch = epoch; });

  expectSendFunds(kBurntFundsActorAddress, 0);

  EXPECT_OUTCOME_TRUE_1(
      MarketActor::HandleExpiredDeals::call(runtime, {{deal_1_id}}));
}

TEST_F(MarketActorTest, HandleExpiredDealsStartTimeout) {
  auto deal = setupHandleExpiredDeals([](auto &) {});

  expectSendFunds(kBurntFundsActorAddress,
                  MarketActor::collateralPenaltyForDealActivationMissed(
                      deal.provider_collateral));

  EXPECT_OUTCOME_TRUE_1(
      MarketActor::HandleExpiredDeals::call(runtime, {{deal_1_id}}));

  EXPECT_OUTCOME_EQ(state.escrow_table.get(miner_address), 0);
  EXPECT_OUTCOME_EQ(state.locked_table.get(miner_address), 0);
  EXPECT_OUTCOME_EQ(state.escrow_table.get(client_address),
                    deal.clientBalanceRequirement());
  EXPECT_OUTCOME_EQ(state.locked_table.get(client_address), 0);
  expectHasDeal(deal_1_id, deal, false);
}

TEST_F(MarketActorTest, HandleExpiredDealsSlashed) {
  auto deal = setupHandleExpiredDeals([](auto &) {});
  DealState deal_state{deal.start_epoch, kChainEpochUndefined, epoch};
  EXPECT_OUTCOME_TRUE_1(state.states.set(deal_1_id, deal_state));

  expectSendFunds(kBurntFundsActorAddress, deal.provider_collateral);

  EXPECT_OUTCOME_TRUE_1(
      MarketActor::HandleExpiredDeals::call(runtime, {{deal_1_id}}));

  auto payment = MarketActor::clientPayment(epoch, deal, deal_state);
  EXPECT_OUTCOME_EQ(state.escrow_table.get(miner_address), payment);
  EXPECT_OUTCOME_EQ(state.locked_table.get(miner_address), 0);
  EXPECT_OUTCOME_EQ(state.escrow_table.get(client_address),
                    deal.clientBalanceRequirement() - payment);
  EXPECT_OUTCOME_EQ(state.locked_table.get(client_address), 0);
  expectHasDeal(deal_1_id, deal, false);
}

TEST_F(MarketActorTest, HandleExpiredDealsEnded) {
  auto deal =
      setupHandleExpiredDeals([&](auto &deal) { deal.end_epoch = epoch; });
  DealState deal_state{
      deal.start_epoch, kChainEpochUndefined, kChainEpochUndefined};
  EXPECT_OUTCOME_TRUE_1(state.states.set(deal_1_id, deal_state));

  expectSendFunds(kBurntFundsActorAddress, 0);

  EXPECT_OUTCOME_TRUE_1(
      MarketActor::HandleExpiredDeals::call(runtime, {{deal_1_id}}));

  EXPECT_OUTCOME_EQ(state.escrow_table.get(miner_address),
                    deal.provider_collateral
                        + MarketActor::clientPayment(epoch, deal, deal_state));
  EXPECT_OUTCOME_EQ(state.locked_table.get(miner_address), 0);
  EXPECT_OUTCOME_EQ(state.escrow_table.get(client_address),
                    deal.client_collateral);
  EXPECT_OUTCOME_EQ(state.locked_table.get(client_address), 0);
  expectHasDeal(deal_1_id, deal, false);
}

TEST_F(MarketActorTest, HandleExpiredDealsUpdated) {
  auto deal = setupHandleExpiredDeals([](auto &) {});
  DealState deal_state{
      deal.start_epoch, kChainEpochUndefined, kChainEpochUndefined};
  EXPECT_OUTCOME_TRUE_1(state.states.set(deal_1_id, deal_state));

  expectSendFunds(kBurntFundsActorAddress, 0);

  EXPECT_OUTCOME_TRUE_1(
      MarketActor::HandleExpiredDeals::call(runtime, {{deal_1_id}}));

  auto payment = MarketActor::clientPayment(epoch, deal, deal_state);
  EXPECT_OUTCOME_EQ(state.escrow_table.get(miner_address),
                    deal.provider_collateral + payment);
  EXPECT_OUTCOME_EQ(state.locked_table.get(miner_address),
                    deal.provider_collateral);
  EXPECT_OUTCOME_EQ(state.escrow_table.get(client_address),
                    deal.clientBalanceRequirement() - payment);
  EXPECT_OUTCOME_EQ(state.locked_table.get(client_address),
                    deal.clientBalanceRequirement() - payment);
  expectHasDeal(deal_1_id, deal, true);
  EXPECT_OUTCOME_TRUE(deal_state_1, state.states.get(deal_1_id));
  EXPECT_EQ(deal_state_1.last_updated_epoch, epoch);
}

ClientDealProposal MarketActorTest::setupPublishStorageDeals() {
  ClientDealProposal proposal;
  auto &deal = proposal.proposal;
  auto duration = MarketActor::dealDurationBounds(deal.piece_size).min + 1;
  deal.piece_cid = some_cid;
  deal.piece_size = 3;
  deal.start_epoch = epoch;
  deal.end_epoch = deal.start_epoch + duration;
  deal.storage_price_per_epoch =
      MarketActor::dealPricePerEpochBounds(deal.piece_size, duration).min + 1;
  deal.provider_collateral =
      MarketActor::dealProviderCollateralBounds(deal.piece_size, duration).min
      + 1;
  deal.client_collateral =
      MarketActor::dealClientCollateralBounds(deal.piece_size, duration).min
      + 1;
  deal.provider = miner_address;
  deal.client = client_address;

  EXPECT_OUTCOME_TRUE_1(
      state.escrow_table.set(miner_address, deal.providerBalanceRequirement()));
  EXPECT_OUTCOME_TRUE_1(state.locked_table.set(miner_address, 0));
  EXPECT_OUTCOME_TRUE_1(
      state.escrow_table.set(client_address, deal.clientBalanceRequirement()));
  EXPECT_OUTCOME_TRUE_1(state.locked_table.set(client_address, 0));

  callerIs(worker_address);
  runtime.expectSendM<MinerActor::ControlAddresses>(
      miner_address, {}, 0, {owner_address, worker_address});
  ON_CALL_3(runtime,
            verifySignature(testing::_, client_address, testing::_),
            fc::outcome::success(true));
  ON_CALL_3(
      runtime,
      verifySignature(testing::_, testing::Not(client_address), testing::_),
      fc::outcome::success(false));

  return proposal;
}

TEST_F(MarketActorTest, PublishStorageDealsNoDeals) {
  callerIs(owner_address);

  EXPECT_OUTCOME_ERROR(VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
                       MarketActor::PublishStorageDeals::call(runtime, {{}}));
}

TEST_F(MarketActorTest, PublishStorageDealsCallerNotWorker) {
  ClientDealProposal proposal;
  proposal.proposal.piece_cid = some_cid;
  proposal.proposal.provider = miner_address;

  callerIs(client_address);
  runtime.expectSendM<MinerActor::ControlAddresses>(
      miner_address, {}, 0, {owner_address, worker_address});

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_FORBIDDEN,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, PublishStorageDealsNonPositiveDuration) {
  auto proposal = setupPublishStorageDeals();
  proposal.proposal.end_epoch = proposal.proposal.start_epoch;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, PublishStorageDealsWrongClientSignature) {
  auto proposal = setupPublishStorageDeals();
  proposal.proposal.client = owner_address;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, PublishStorageDealsStartTimeout) {
  auto proposal = setupPublishStorageDeals();
  proposal.proposal.start_epoch = epoch - 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, PublishStorageDealsDurationOutOfBounds) {
  auto proposal = setupPublishStorageDeals();
  auto &deal = proposal.proposal;
  deal.end_epoch = deal.start_epoch
                   + MarketActor::dealDurationBounds(deal.piece_size).max + 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, PublishStorageDealsPricePerEpochOutOfBounds) {
  auto proposal = setupPublishStorageDeals();
  auto &deal = proposal.proposal;
  deal.storage_price_per_epoch =
      MarketActor::dealPricePerEpochBounds(deal.piece_size, deal.duration()).max
      + 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, PublishStorageDealsProviderCollateralOutOfBounds) {
  auto proposal = setupPublishStorageDeals();
  auto &deal = proposal.proposal;
  deal.provider_collateral = MarketActor::dealProviderCollateralBounds(
                                 deal.piece_size, deal.duration())
                                 .max
                             + 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, PublishStorageDealsClientCollateralOutOfBounds) {
  auto proposal = setupPublishStorageDeals();
  auto &deal = proposal.proposal;
  deal.client_collateral =
      MarketActor::dealClientCollateralBounds(deal.piece_size, deal.duration())
          .max
      + 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, PublishStorageDealsDifferentProviders) {
  auto proposal = setupPublishStorageDeals();
  auto proposal2 = proposal;
  proposal2.proposal.provider = client_address;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal, proposal2}}));
}

TEST_F(MarketActorTest, PublishStorageDealsProviderInsufficientBalance) {
  auto proposal = setupPublishStorageDeals();

  EXPECT_OUTCOME_TRUE_1(state.escrow_table.set(miner_address, 0));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_INSUFFICIENT_FUNDS,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, PublishStorageDealsClientInsufficientBalance) {
  auto proposal = setupPublishStorageDeals();

  EXPECT_OUTCOME_TRUE_1(state.escrow_table.set(client_address, 0));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_INSUFFICIENT_FUNDS,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, PublishStorageDeals) {
  auto proposal = setupPublishStorageDeals();
  auto &deal = proposal.proposal;
  state.next_deal = deal_1_id;

  expectSendFunds(kBurntFundsActorAddress, 0);

  EXPECT_OUTCOME_TRUE(
      result, MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));

  EXPECT_THAT(result.deals, testing::ElementsAre(deal_1_id));
  EXPECT_EQ(state.next_deal, deal_1_id + 1);
  expectHasDeal(deal_1_id, deal, true);
  EXPECT_OUTCOME_EQ(state.locked_table.get(miner_address),
                    deal.providerBalanceRequirement());
  EXPECT_OUTCOME_EQ(state.locked_table.get(client_address),
                    deal.clientBalanceRequirement());
}

DealProposal MarketActorTest::setupVerifyDealsOnSectorProveCommit(
    const std::function<void(DealProposal &)> &prepare) {
  DealProposal deal;
  deal.piece_size = 3;
  deal.piece_cid = some_cid;
  deal.provider = miner_address;
  deal.start_epoch = epoch;
  deal.end_epoch = deal.start_epoch + 10;
  prepare(deal);
  EXPECT_OUTCOME_TRUE_1(state.proposals.set(deal_1_id, deal));

  callerIs(miner_address);

  return deal;
}

TEST_F(MarketActorTest, VerifyDealsOnSectorProveCommitCallerNotMiner) {
  callerIs(client_address);

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_WRONG_CALLER,
      MarketActor::VerifyDealsOnSectorProveCommit::call(runtime, {{}, {}}));
}

TEST_F(MarketActorTest, VerifyDealsOnSectorProveCommitNotProvider) {
  auto deal = setupVerifyDealsOnSectorProveCommit(
      [&](auto &deal) { deal.provider = client_address; });

  EXPECT_OUTCOME_ERROR(VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
                       MarketActor::VerifyDealsOnSectorProveCommit::call(
                           runtime, {{deal_1_id}, {}}));
}

TEST_F(MarketActorTest, VerifyDealsOnSectorProveCommitAlreadyStarted) {
  auto deal = setupVerifyDealsOnSectorProveCommit([](auto &) {});
  EXPECT_OUTCOME_TRUE_1(state.states.set(deal_1_id, {1, {}, {}}));

  EXPECT_OUTCOME_ERROR(VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
                       MarketActor::VerifyDealsOnSectorProveCommit::call(
                           runtime, {{deal_1_id}, {}}));
}

TEST_F(MarketActorTest, VerifyDealsOnSectorProveCommitStartTimeout) {
  auto deal = setupVerifyDealsOnSectorProveCommit(
      [&](auto &deal) { deal.start_epoch = epoch - 1; });

  EXPECT_OUTCOME_ERROR(VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
                       MarketActor::VerifyDealsOnSectorProveCommit::call(
                           runtime, {{deal_1_id}, {}}));
}

TEST_F(MarketActorTest, VerifyDealsOnSectorProveCommitSectorEndsBeforeDeal) {
  auto deal = setupVerifyDealsOnSectorProveCommit([](auto &) {});

  EXPECT_OUTCOME_ERROR(VMExitCode::MARKET_ACTOR_ILLEGAL_ARGUMENT,
                       MarketActor::VerifyDealsOnSectorProveCommit::call(
                           runtime, {{deal_1_id}, deal.end_epoch - 1}));
}

TEST_F(MarketActorTest, VerifyDealsOnSectorProveCommit) {
  auto deal = setupVerifyDealsOnSectorProveCommit([](auto &) {});

  EXPECT_OUTCOME_EQ(MarketActor::VerifyDealsOnSectorProveCommit::call(
                        runtime, {{deal_1_id}, deal.end_epoch}),
                    deal.piece_size * deal.duration());

  EXPECT_OUTCOME_TRUE(deal_state, state.states.get(deal_1_id));
  EXPECT_EQ(deal_state.sector_start_epoch, epoch);
}

TEST_F(MarketActorTest, OnMinerSectorsTerminateNotDealMiner) {
  DealProposal deal;
  deal.piece_cid = some_cid;
  deal.provider = client_address;
  EXPECT_OUTCOME_TRUE_1(state.proposals.set(deal_1_id, deal));

  callerIs(miner_address);

  EXPECT_OUTCOME_ERROR(
      VMExitCode::MARKET_ACTOR_FORBIDDEN,
      MarketActor::OnMinerSectorsTerminate::call(runtime, {{deal_1_id}}));
}

TEST_F(MarketActorTest, OnMinerSectorsTerminate) {
  DealProposal deal;
  deal.piece_cid = some_cid;
  deal.provider = miner_address;
  EXPECT_OUTCOME_TRUE_1(state.proposals.set(deal_1_id, deal));

  callerIs(miner_address);
  EXPECT_OUTCOME_TRUE_1(
      MarketActor::OnMinerSectorsTerminate::call(runtime, {{deal_1_id}}));

  EXPECT_OUTCOME_TRUE(deal_state, state.states.get(deal_1_id));
  EXPECT_EQ(deal_state.slash_epoch, epoch);
}

TEST_F(MarketActorTest, ComputeDataCommitmentCallerNotMiner) {
  callerIs(client_address);

  EXPECT_OUTCOME_ERROR(VMExitCode::MARKET_ACTOR_WRONG_CALLER,
                       MarketActor::ComputeDataCommitment::call(runtime, {}));
}

TEST_F(MarketActorTest, ComputeDataCommitment) {
  auto comm_d = "010001020001"_cid;
  auto sector_type = RegisteredProof::StackedDRG32GiBSeal;
  std::vector<DealId> deal_ids{deal_1_id, deal_2_id};
  std::vector<PieceInfo> pieces{
      {PaddedPieceSize(31), "010001020002"_cid},
      {PaddedPieceSize(42), "010001020003"_cid},
  };

  for (auto i = 0u; i < deal_ids.size(); ++i) {
    DealProposal deal;
    deal.piece_cid = pieces[i].cid;
    deal.piece_size = pieces[i].size;
    EXPECT_OUTCOME_TRUE_1(state.proposals.set(deal_ids[i], deal));
  }

  callerIs(miner_address);
  EXPECT_CALL(runtime, computeUnsealedSectorCid(sector_type, pieces))
      .WillOnce(Return(comm_d));

  EXPECT_OUTCOME_EQ(MarketActor::ComputeDataCommitment::call(
                        runtime, {deal_ids, sector_type}),
                    comm_d);
}
