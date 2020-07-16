/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/predef/compiler/gcc.h>

// TODO: some tests fail on gcc
#if BOOST_COMP_GNUC
#define GCC_DISABLE(name) DISABLED_##name
#else
#define GCC_DISABLE(name) name
#endif

#include "vm/actor/builtin/market/actor.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "testutil/crypto/sample_signatures.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/actor/builtin/market/policy.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

#define ON_CALL_3(a, b, c) \
  EXPECT_CALL(a, b).Times(testing::AnyNumber()).WillRepeatedly(Return(c))

namespace MarketActor = fc::vm::actor::builtin::market;
namespace MinerActor = fc::vm::actor::builtin::miner;
using fc::crypto::bls::Signature;
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
using fc::vm::actor::kAccountCodeCid;
using fc::vm::actor::kBurntFundsActorAddress;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::kInitCodeCid;
using fc::vm::actor::kSendMethodNumber;
using fc::vm::actor::kStorageMinerCodeCid;
using fc::vm::actor::kSystemActorAddress;
using fc::vm::runtime::MockRuntime;
using fc::vm::state::StateTreeImpl;
using MarketActor::ClientDealProposal;
using MarketActor::DealProposal;
using MarketActor::DealState;
using MarketActor::State;
using testing::Return;

const auto some_cid = "01000102ffff"_cid;
DealId deal_1_id = 13, deal_2_id = 24;

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
                  .verified = false,
                  .client = Address::makeFromId(1),
                  .provider = Address::makeFromId(2),
                  .start_epoch = 2,
                  .end_epoch = 3,
                  .storage_price_per_epoch = 4,
                  .provider_collateral = 5,
                  .client_collateral = 6,
              },
          .client_signature = kSampleSecp256k1Signature,
      },
      "828ad82a470001000102000101f4420001420002020342000442000542000"
      "6" SAMPLE_SECP256K1_SIGNATURE_HEX ""_unhex);
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

    ipld->load(state);

    EXPECT_CALL(runtime, getCurrentActorState())
        .Times(testing::AtMost(1))
        .WillOnce(testing::Invoke([&]() {
          EXPECT_OUTCOME_TRUE(cid, ipld->setCbor(state));
          return std::move(cid);
        }));

    EXPECT_CALL(runtime, commit(testing::_))
        .Times(testing::AtMost(1))
        .WillOnce(testing::Invoke([&](auto &cid) {
          EXPECT_OUTCOME_TRUE(new_state,
                              ipld->getCbor<MarketActor::State>(cid));
          state = std::move(new_state);
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

  void expectHasDeal(DealId deal_id, const DealProposal &deal, bool has) {
    if (has) {
      EXPECT_OUTCOME_EQ(state.proposals.get(deal_id), deal);
    } else {
      EXPECT_OUTCOME_EQ(state.proposals.has(deal_id), has);
    }
  }

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

  EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrForbidden,
                       MarketActor::Construct::call(runtime, {}));
}

TEST_F(MarketActorTest, Constructor) {
  callerIs(kSystemActorAddress);

  EXPECT_OUTCOME_TRUE_1(MarketActor::Construct::call(runtime, {}));
}

TEST_F(MarketActorTest, AddBalanceNominalNotSignable) {
  callerIs(kInitAddress);

  EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrForbidden,
                       MarketActor::AddBalance::call(runtime, kInitAddress));
}

TEST_F(MarketActorTest, AddBalanceNominalNotOwnerOrWorker) {
  callerIs(kInitAddress);

  runtime.expectSendM<MinerActor::ControlAddresses>(
      miner_address, {}, 0, {owner_address, worker_address});

  EXPECT_OUTCOME_ERROR(VMExitCode::kMarketActorWrongCaller,
                       MarketActor::AddBalance::call(runtime, miner_address));
}

TEST_F(MarketActorTest, AddBalance) {
  TokenAmount amount{100};

  callerIs(owner_address);
  EXPECT_CALL(runtime, getValueReceived()).WillOnce(Return(amount));

  EXPECT_OUTCOME_TRUE_1(MarketActor::AddBalance::call(runtime, client_address));

  EXPECT_OUTCOME_EQ(state.escrow_table.get(client_address), amount);
  EXPECT_OUTCOME_EQ(state.locked_table.get(client_address), 0);
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

  EXPECT_OUTCOME_ERROR(VMExitCode::kAssert,
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
      VMExitCode::kSysErrForbidden,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, GCC_DISABLE(PublishStorageDealsNonPositiveDuration)) {
  auto proposal = setupPublishStorageDeals();
  proposal.proposal.end_epoch = proposal.proposal.start_epoch;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kMarketActorIllegalArgument,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, GCC_DISABLE(PublishStorageDealsWrongClientSignature)) {
  auto proposal = setupPublishStorageDeals();
  proposal.proposal.client = owner_address;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kMarketActorIllegalArgument,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, GCC_DISABLE(PublishStorageDealsStartTimeout)) {
  auto proposal = setupPublishStorageDeals();
  proposal.proposal.start_epoch = epoch - 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kMarketActorIllegalArgument,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, GCC_DISABLE(PublishStorageDealsDurationOutOfBounds)) {
  auto proposal = setupPublishStorageDeals();
  auto &deal = proposal.proposal;
  deal.end_epoch = deal.start_epoch
                   + MarketActor::dealDurationBounds(deal.piece_size).max + 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kMarketActorIllegalArgument,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest,
       GCC_DISABLE(PublishStorageDealsPricePerEpochOutOfBounds)) {
  auto proposal = setupPublishStorageDeals();
  auto &deal = proposal.proposal;
  deal.storage_price_per_epoch =
      MarketActor::dealPricePerEpochBounds(deal.piece_size, deal.duration()).max
      + 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kMarketActorIllegalArgument,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest,
       GCC_DISABLE(PublishStorageDealsProviderCollateralOutOfBounds)) {
  auto proposal = setupPublishStorageDeals();
  auto &deal = proposal.proposal;
  deal.provider_collateral = MarketActor::dealProviderCollateralBounds(
                                 deal.piece_size, deal.duration())
                                 .max
                             + 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kMarketActorIllegalArgument,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest,
       GCC_DISABLE(PublishStorageDealsClientCollateralOutOfBounds)) {
  auto proposal = setupPublishStorageDeals();
  auto &deal = proposal.proposal;
  deal.client_collateral =
      MarketActor::dealClientCollateralBounds(deal.piece_size, deal.duration())
          .max
      + 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kMarketActorIllegalArgument,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, GCC_DISABLE(PublishStorageDealsDifferentProviders)) {
  auto proposal = setupPublishStorageDeals();
  auto proposal2 = proposal;
  proposal2.proposal.provider = client_address;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kAssert,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal, proposal2}}));
}

TEST_F(MarketActorTest,
       GCC_DISABLE(PublishStorageDealsProviderInsufficientBalance)) {
  auto proposal = setupPublishStorageDeals();

  EXPECT_OUTCOME_TRUE_1(state.escrow_table.set(miner_address, 0));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kMarketActorInsufficientFunds,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest,
       GCC_DISABLE(PublishStorageDealsClientInsufficientBalance)) {
  auto proposal = setupPublishStorageDeals();

  EXPECT_OUTCOME_TRUE_1(state.escrow_table.set(client_address, 0));

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kMarketActorInsufficientFunds,
      MarketActor::PublishStorageDeals::call(runtime, {{proposal}}));
}

TEST_F(MarketActorTest, GCC_DISABLE(PublishStorageDeals)) {
  auto proposal = setupPublishStorageDeals();
  auto &deal = proposal.proposal;
  state.next_deal = deal_1_id;

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
      VMExitCode::kSysErrForbidden,
      MarketActor::VerifyDealsOnSectorProveCommit::call(runtime, {{}, {}}));
}

TEST_F(MarketActorTest,
       GCC_DISABLE(VerifyDealsOnSectorProveCommitNotProvider)) {
  auto deal = setupVerifyDealsOnSectorProveCommit(
      [&](auto &deal) { deal.provider = client_address; });

  EXPECT_OUTCOME_ERROR(VMExitCode::kAssert,
                       MarketActor::VerifyDealsOnSectorProveCommit::call(
                           runtime, {{deal_1_id}, {}}));
}

TEST_F(MarketActorTest, VerifyDealsOnSectorProveCommitAlreadyStarted) {
  auto deal = setupVerifyDealsOnSectorProveCommit([](auto &) {});
  EXPECT_OUTCOME_TRUE_1(state.states.set(deal_1_id, {1, {}, {}}));

  EXPECT_OUTCOME_ERROR(VMExitCode::kAssert,
                       MarketActor::VerifyDealsOnSectorProveCommit::call(
                           runtime, {{deal_1_id}, {}}));
}

TEST_F(MarketActorTest,
       GCC_DISABLE(VerifyDealsOnSectorProveCommitStartTimeout)) {
  auto deal = setupVerifyDealsOnSectorProveCommit(
      [&](auto &deal) { deal.start_epoch = epoch - 1; });

  EXPECT_OUTCOME_ERROR(VMExitCode::kAssert,
                       MarketActor::VerifyDealsOnSectorProveCommit::call(
                           runtime, {{deal_1_id}, {}}));
}

TEST_F(MarketActorTest,
       GCC_DISABLE(VerifyDealsOnSectorProveCommitSectorEndsBeforeDeal)) {
  auto deal = setupVerifyDealsOnSectorProveCommit([](auto &) {});

  EXPECT_OUTCOME_ERROR(VMExitCode::kAssert,
                       MarketActor::VerifyDealsOnSectorProveCommit::call(
                           runtime, {{deal_1_id}, deal.end_epoch - 1}));
}

TEST_F(MarketActorTest, GCC_DISABLE(VerifyDealsOnSectorProveCommit)) {
  auto deal = setupVerifyDealsOnSectorProveCommit([](auto &) {});

  EXPECT_OUTCOME_TRUE_1(MarketActor::VerifyDealsOnSectorProveCommit::call(
      runtime, {{deal_1_id}, deal.end_epoch}));

  EXPECT_OUTCOME_TRUE(deal_state, state.states.get(deal_1_id));
  EXPECT_EQ(deal_state.sector_start_epoch, epoch);
}

TEST_F(MarketActorTest, GCC_DISABLE(OnMinerSectorsTerminateNotDealMiner)) {
  DealProposal deal;
  deal.piece_cid = some_cid;
  deal.provider = client_address;
  EXPECT_OUTCOME_TRUE_1(state.proposals.set(deal_1_id, deal));

  callerIs(miner_address);

  EXPECT_OUTCOME_ERROR(
      VMExitCode::kAssert,
      MarketActor::OnMinerSectorsTerminate::call(runtime, {{deal_1_id}}));
}

TEST_F(MarketActorTest, GCC_DISABLE(OnMinerSectorsTerminate)) {
  DealProposal deal;
  deal.piece_cid = some_cid;
  deal.provider = miner_address;
  EXPECT_OUTCOME_TRUE_1(state.proposals.set(deal_1_id, deal));
  EXPECT_OUTCOME_TRUE_1(state.states.set(
      deal_1_id,
      {kChainEpochUndefined, kChainEpochUndefined, kChainEpochUndefined}));

  callerIs(miner_address);
  EXPECT_OUTCOME_TRUE_1(
      MarketActor::OnMinerSectorsTerminate::call(runtime, {{deal_1_id}}));

  EXPECT_OUTCOME_TRUE(deal_state, state.states.get(deal_1_id));
  EXPECT_EQ(deal_state.slash_epoch, epoch);
}

TEST_F(MarketActorTest, ComputeDataCommitmentCallerNotMiner) {
  callerIs(client_address);

  EXPECT_OUTCOME_ERROR(VMExitCode::kSysErrForbidden,
                       MarketActor::ComputeDataCommitment::call(runtime, {}));
}

TEST_F(MarketActorTest, GCC_DISABLE(ComputeDataCommitment)) {
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
