/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/payment_channel/payment_channel_actor.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

#define ON_CALL_3(object, call, result) \
  EXPECT_CALL(object, call)             \
      .Times(testing::AnyNumber())      \
      .WillRepeatedly(Return(result))

namespace PaymentChannel = fc::vm::actor::builtin::payment_channel;
using fc::common::Buffer;
using fc::crypto::blake2b::blake2b_256;
using fc::crypto::signature::Secp256k1Signature;
using fc::primitives::ChainEpoch;
using fc::primitives::TokenAmount;
using fc::primitives::address::Address;
using fc::storage::ipfs::InMemoryDatastore;
using fc::vm::VMExitCode;
using fc::vm::actor::ActorSubstateCID;
using fc::vm::actor::kAccountCodeCid;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::kInitCodeCid;
using fc::vm::actor::kSendMethodNumber;
using fc::vm::runtime::MockRuntime;
using fc::vm::state::StateTreeImpl;
using PaymentChannel::LaneState;
using PaymentChannel::Merge;
using testing::Return;

struct PaymentChannelActorTest : testing::Test {
  void SetUp() override {
    ON_CALL_3(runtime, getIpfsDatastore(), ipld);

    EXPECT_CALL(runtime, resolveAddress(testing::_))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Invoke(
            [&](auto &address) { return state_tree.lookupId(address); }));

    ON_CALL_3(runtime, getCurrentEpoch(), epoch);

    EXPECT_CALL(runtime, getBalance(actor_address))
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Invoke(
            [&](auto &) { return fc::outcome::success(balance); }));

    EXPECT_CALL(runtime, getImmediateCaller())
        .Times(testing::AnyNumber())
        .WillRepeatedly(testing::Invoke([&]() { return caller; }));

    ON_CALL_3(runtime, getCurrentReceiver(), actor_address);

    ON_CALL_3(runtime, getActorCodeID(kInitAddress), kInitCodeCid);
    ON_CALL_3(runtime, getActorCodeID(from_address), kAccountCodeCid);
    ON_CALL_3(runtime, getActorCodeID(to_address), kAccountCodeCid);

    EXPECT_CALL(runtime, getCurrentActorState())
        .Times(testing::AtMost(1))
        .WillOnce(testing::Invoke([&]() {
          EXPECT_OUTCOME_TRUE(cid, ipld->setCbor(state));
          return ActorSubstateCID{std::move(cid)};
        }));

    EXPECT_CALL(runtime, commit(testing::_))
        .Times(testing::AtMost(1))
        .WillOnce(testing::Invoke([&](auto &cid) {
          EXPECT_OUTCOME_TRUE(new_state,
                              ipld->getCbor<PaymentChannel::State>(cid));
          state = std::move(new_state);
          return fc::outcome::success();
        }));
  }

  void expectSendFunds(const Address &address, TokenAmount amount) {
    EXPECT_CALL(runtime, send(address, kSendMethodNumber, testing::_, amount))
        .WillOnce(testing::Invoke([&](auto &m, auto, auto &, auto amount) {
          balance -= amount;
          return fc::outcome::success();
        }));
  }

  void setupState() {
    state.from = from_address;
    state.to = to_address;
    state.settling_at = 0;
    state.min_settling_height = 0;
  }

  PaymentChannel::SignedVoucher setupUpdateChannelState();

  MockRuntime runtime;

  std::shared_ptr<InMemoryDatastore> ipld{
      std::make_shared<InMemoryDatastore>()};

  ChainEpoch epoch{2077};

  TokenAmount balance;

  Address caller;

  Address from_address{Address::makeFromId(101)};
  Address to_address{Address::makeFromId(102)};
  Address actor_address{Address::makeFromId(103)};

  PaymentChannel::State state;

  StateTreeImpl state_tree{ipld};
};

/// PaymentChannelActor Construct error: caller is not init actor
TEST_F(PaymentChannelActorTest, ConstructCallerNotInit) {
  caller = from_address;

  EXPECT_OUTCOME_ERROR(VMExitCode::PAYMENT_CHANNEL_WRONG_CALLER,
                       PaymentChannel::Construct::call(runtime, {}));
}

/// PaymentChannelActor Construct error: "to" is not account actor
TEST_F(PaymentChannelActorTest, ConstructToNotAccount) {
  caller = kInitAddress;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannel::Construct::call(runtime, {{}, kInitAddress}));
}

/// PaymentChannelActor Construct error: "from" is not account actor
TEST_F(PaymentChannelActorTest, ConstructFromNotAccount) {
  caller = kInitAddress;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannel::Construct::call(runtime, {kInitAddress, to_address}));
}

/// PaymentChannelActor Construct success
TEST_F(PaymentChannelActorTest, ConstructSuccess) {
  caller = kInitAddress;

  EXPECT_OUTCOME_TRUE_1(
      PaymentChannel::Construct::call(runtime, {from_address, to_address}));

  EXPECT_EQ(state.from, from_address);
  EXPECT_EQ(state.to, to_address);
  EXPECT_EQ(state.to_send, 0);
  EXPECT_EQ(state.settling_at, 0);
  EXPECT_EQ(state.min_settling_height, 0);
  EXPECT_EQ(state.lanes.size(), 0);
}

PaymentChannel::SignedVoucher
PaymentChannelActorTest::setupUpdateChannelState() {
  setupState();

  PaymentChannel::SignedVoucher voucher;
  caller = from_address;
  voucher.signature = Secp256k1Signature{};
  const auto &sig = *voucher.signature;
  ON_CALL_3(runtime,
            verifySignature(sig, testing::_, testing::_),
            fc::outcome::success(true));
  ON_CALL_3(runtime,
            verifySignature(testing::Not(sig), testing::_, testing::_),
            fc::outcome::success(false));
  voucher.time_lock_min = epoch;
  voucher.time_lock_max = epoch;
  voucher.lane = 100;
  voucher.nonce = 10;
  voucher.amount = 100;
  return voucher;
}

/// PaymentChannelActor UpdateChannelState error: voucher has no signature
TEST_F(PaymentChannelActorTest, UpdateChannelStateNoSignature) {
  auto voucher = setupUpdateChannelState();
  voucher.signature = boost::none;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));
}

/// PaymentChannelActor UpdateChannelState error: invalid voucher signature
TEST_F(PaymentChannelActorTest, UpdateChannelStateSignatureNotVerified) {
  auto voucher = setupUpdateChannelState();
  voucher.signature = Secp256k1Signature{1};

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));
}

/// PaymentChannelActor UpdateChannelState error: epoch before voucher min
TEST_F(PaymentChannelActorTest, UpdateChannelStateBeforeMin) {
  auto voucher = setupUpdateChannelState();
  voucher.time_lock_min = epoch + 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));
}

/// PaymentChannelActor UpdateChannelState error: epoch after voucher max
TEST_F(PaymentChannelActorTest, UpdateChannelStateAfterMax) {
  auto voucher = setupUpdateChannelState();
  voucher.time_lock_max = epoch - 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));
}

/// PaymentChannelActor UpdateChannelState error: invalid secret preimage
TEST_F(PaymentChannelActorTest, UpdateChannelStateInvalidSecretPreimage) {
  auto voucher = setupUpdateChannelState();
  voucher.secret_preimage = Buffer{blake2b_256("01"_unhex)};

  EXPECT_OUTCOME_ERROR(VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
                       PaymentChannel::UpdateChannelState::call(
                           runtime, {voucher, Buffer{"02"_unhex}, {}}));
}

/// PaymentChannelActor UpdateChannelState error: extra call failed
TEST_F(PaymentChannelActorTest, UpdateChannelStateExtraFailed) {
  auto voucher = setupUpdateChannelState();
  voucher.extra = PaymentChannel::ModularVerificationParameter{
      state.to,
      123,
      {},
  };
  EXPECT_OUTCOME_TRUE(
      params,
      fc::vm::actor::encodeActorParams(PaymentChannel::PaymentVerifyParams{
          voucher.extra->data, Buffer{"01"_unhex}}));
  EXPECT_CALL(
      runtime,
      send(voucher.extra->actor, voucher.extra->method, params, TokenAmount{0}))
      .WillOnce(Return(VMExitCode::CRON_ACTOR_WRONG_CALL));

  EXPECT_OUTCOME_ERROR(VMExitCode::CRON_ACTOR_WRONG_CALL,
                       PaymentChannel::UpdateChannelState::call(
                           runtime, {voucher, {}, Buffer{"01"_unhex}}));
}

/// PaymentChannelActor UpdateChannelState error: lane limit exceeded
TEST_F(PaymentChannelActorTest, UpdateChannelStateLaneLimit) {
  auto voucher = setupUpdateChannelState();
  state.lanes.clear();
  state.lanes.resize(PaymentChannel::kLaneLimit, LaneState{});

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));
}

/// PaymentChannelActor UpdateChannelState error: expired voucher lane nonce
TEST_F(PaymentChannelActorTest, UpdateChannelStateInvalidVoucherNonce) {
  auto voucher = setupUpdateChannelState();
  state.lanes.push_back(LaneState{voucher.lane, {}, voucher.nonce + 1});

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));
}

/// PaymentChannelActor UpdateChannelState error: voucher merges to own lane
TEST_F(PaymentChannelActorTest, UpdateChannelStateMergeSelf) {
  auto voucher = setupUpdateChannelState();
  voucher.merges.push_back(Merge{voucher.lane, {}});

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));
}

/// PaymentChannelActor UpdateChannelState error: expired voucher merge lane
/// nonce
TEST_F(PaymentChannelActorTest, UpdateChannelStateInvalidMergeNonce) {
  auto voucher = setupUpdateChannelState();
  LaneState lane{102, {}, 5};
  state.lanes.push_back(lane);
  voucher.merges.push_back(Merge{lane.id, lane.nonce});

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_ARGUMENT,
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));
}

/// PaymentChannelActor UpdateChannelState error: "to send" is negative
TEST_F(PaymentChannelActorTest, UpdateChannelStateNegative) {
  auto voucher = setupUpdateChannelState();
  state.to_send = 10;
  state.lanes.push_back(LaneState{
      voucher.lane, voucher.amount + state.to_send + 1, voucher.nonce});

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_STATE,
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));
}

/// PaymentChannelActor UpdateChannelState error: "to send" exceeds balance
TEST_F(PaymentChannelActorTest, UpdateChannelStateAboveBalance) {
  auto voucher = setupUpdateChannelState();
  state.to_send = 10;
  balance = state.to_send + voucher.amount - 1;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::PAYMENT_CHANNEL_ILLEGAL_STATE,
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));
}

/// PaymentChannelActor UpdateChannelState success
TEST_F(PaymentChannelActorTest, UpdateChannelState) {
  auto voucher = setupUpdateChannelState();
  auto to_send = 10;
  state.to_send = to_send;
  balance = state.to_send + voucher.amount;

  EXPECT_OUTCOME_TRUE_1(
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));

  EXPECT_EQ(state.to_send, to_send + voucher.amount);
}

/// PaymentChannelActor UpdateChannelState success: settling round up to min
TEST_F(PaymentChannelActorTest, UpdateChannelStateMinHeight) {
  auto voucher = setupUpdateChannelState();
  voucher.min_close_height = 1000;
  state.settling_at = 1;
  balance = voucher.amount;

  EXPECT_OUTCOME_TRUE_1(
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));

  EXPECT_EQ(state.settling_at, voucher.min_close_height);
  EXPECT_EQ(state.min_settling_height, voucher.min_close_height);
}

/// PaymentChannelActor UpdateChannelState success: voucher with merge
TEST_F(PaymentChannelActorTest, UpdateChannelStateMerge) {
  auto voucher = setupUpdateChannelState();
  LaneState lane{102, {}, 5};
  state.lanes.push_back(lane);
  voucher.merges.push_back(Merge{lane.id, lane.nonce + 1});
  auto to_send = voucher.amount - lane.redeem;
  balance = to_send;

  EXPECT_OUTCOME_TRUE_1(
      PaymentChannel::UpdateChannelState::call(runtime, {voucher, {}, {}}));

  EXPECT_EQ(state.to_send, to_send);
  EXPECT_EQ(state.findLane(voucher.lane)->redeem, voucher.amount);
}

/// PaymentChannelActor Settle error: caller not in channel
TEST_F(PaymentChannelActorTest, SettleCallerNotInChannel) {
  setupState();
  caller = kInitAddress;

  EXPECT_OUTCOME_ERROR(VMExitCode::PAYMENT_CHANNEL_WRONG_CALLER,
                       PaymentChannel::Settle::call(runtime, {}));
}

/// PaymentChannelActor Settle success: epoch round up to min
TEST_F(PaymentChannelActorTest, SettleBelowMin) {
  setupState();
  state.min_settling_height = epoch + PaymentChannel::kSettleDelay + 1;
  caller = from_address;

  EXPECT_OUTCOME_TRUE_1(PaymentChannel::Settle::call(runtime, {}));

  EXPECT_EQ(state.settling_at, state.min_settling_height);
}

/// PaymentChannelActor Settle success
TEST_F(PaymentChannelActorTest, Settle) {
  setupState();
  state.min_settling_height = epoch + PaymentChannel::kSettleDelay - 1;
  caller = from_address;

  EXPECT_OUTCOME_TRUE_1(PaymentChannel::Settle::call(runtime, {}));

  EXPECT_EQ(state.settling_at, epoch + PaymentChannel::kSettleDelay);
}

/// PaymentChannelActor Collect error: not settling
TEST_F(PaymentChannelActorTest, CollectNotSettling) {
  setupState();
  caller = from_address;

  EXPECT_OUTCOME_ERROR(VMExitCode::PAYMENT_CHANNEL_FORBIDDEN,
                       PaymentChannel::Collect::call(runtime, {}));
}

/// PaymentChannelActor Collect error: epoch before settled
TEST_F(PaymentChannelActorTest, CollectBeforeSettled) {
  setupState();
  state.settling_at = epoch + 1;
  caller = from_address;

  EXPECT_OUTCOME_ERROR(VMExitCode::PAYMENT_CHANNEL_FORBIDDEN,
                       PaymentChannel::Collect::call(runtime, {}));
}

/// PaymentChannelActor Collect success
TEST_F(PaymentChannelActorTest, Collect) {
  setupState();
  state.settling_at = epoch;
  state.to_send = 150;
  balance = 200;
  caller = from_address;
  expectSendFunds(from_address, balance - state.to_send);
  expectSendFunds(to_address, state.to_send);

  EXPECT_OUTCOME_TRUE_1(PaymentChannel::Collect::call(runtime, {}));

  EXPECT_EQ(state.to_send, 0);
}
