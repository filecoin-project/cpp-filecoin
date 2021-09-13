/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/states/payment_channel/v2/payment_channel_actor_state.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/codes.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

#define ON_CALL_3(object, call, result) \
  EXPECT_CALL(object, call).WillRepeatedly(Return(result))

namespace fc::vm::actor::builtin::v2::payment_channel {
  using common::Buffer;
  using crypto::blake2b::blake2b_256;
  using crypto::signature::Secp256k1Signature;
  using crypto::signature::Signature;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using runtime::MockRuntime;
  using state::StateTreeImpl;
  using storage::ipfs::InMemoryDatastore;
  using testing::Return;
  using types::payment_channel::kSettleDelay;
  using types::payment_channel::LaneId;
  using types::payment_channel::LaneState;
  using types::payment_channel::Merge;
  using types::payment_channel::ModularVerificationParameter;
  using types::payment_channel::PaymentVerifyParams;
  using types::payment_channel::SignedVoucher;

  struct PaymentChannelActorTest : testing::Test {
    void SetUp() override {
      actor_version = ActorVersion::kVersion2;
      ipld->actor_version = actor_version;
      cbor_blake::cbLoadT(ipld, state);

      EXPECT_CALL(runtime, getActorVersion())
          .WillRepeatedly(testing::Invoke([&]() { return actor_version; }));

      ON_CALL_3(runtime, getIpfsDatastore(), ipld);

      runtime.resolveAddressWith(state_tree);

      ON_CALL_3(runtime, getCurrentEpoch(), epoch);

      EXPECT_CALL(runtime, getBalance(actor_address))
          .WillRepeatedly(testing::Invoke(
              [&](auto &) { return fc::outcome::success(balance); }));

      EXPECT_CALL(runtime, getImmediateCaller())
          .WillRepeatedly(testing::Invoke([&]() { return caller; }));

      ON_CALL_3(runtime, getCurrentReceiver(), actor_address);

      ON_CALL_3(runtime, getActorCodeID(kInitAddress), kInitCodeId);
      ON_CALL_3(runtime, getActorCodeID(from_address), kAccountCodeId);
      ON_CALL_3(runtime, getActorCodeID(to_address), kAccountCodeId);

      EXPECT_CALL(runtime, hashBlake2b(testing::_))
          .WillRepeatedly(
              testing::Invoke([&](auto &data) { return blake2b_256(data); }));

      EXPECT_CALL(runtime, commit(testing::_))
          .WillRepeatedly(testing::Invoke([&](auto &cid) {
            EXPECT_OUTCOME_TRUE(new_state,
                                getCbor<PaymentChannelActorState>(ipld, cid));
            state = std::move(new_state);
            return outcome::success();
          }));

      EXPECT_CALL(runtime, getActorStateCid())
          .WillRepeatedly(testing::Invoke([&]() {
            EXPECT_OUTCOME_TRUE(cid, setCbor(ipld, state));
            return std::move(cid);
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

    SignedVoucher setupUpdateChannelState();

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};

    ChainEpoch epoch{2077};

    TokenAmount balance;

    Address caller;

    Address from_address{Address::makeFromId(101)};
    Address to_address{Address::makeFromId(102)};
    Address actor_address{Address::makeFromId(103)};

    PaymentChannelActorState state;

    StateTreeImpl state_tree{ipld};
    ActorVersion actor_version;
  };

  /// PaymentChannelActor Construct error: caller is not init actor
  TEST_F(PaymentChannelActorTest, ConstructCallerNotInit) {
    caller = from_address;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         Construct::call(runtime, {}));
  }

  /// PaymentChannelActor Construct error: "to" is not account actor
  TEST_F(PaymentChannelActorTest, ConstructToNotAccount) {
    caller = kInitAddress;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         Construct::call(runtime, {{}, kInitAddress}));
  }

  /// PaymentChannelActor Construct error: "from" is not account actor
  TEST_F(PaymentChannelActorTest, ConstructFromNotAccount) {
    caller = kInitAddress;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         Construct::call(runtime, {kInitAddress, to_address}));
  }

  /// PaymentChannelActor Construct success
  TEST_F(PaymentChannelActorTest, ConstructSuccess) {
    caller = kInitAddress;

    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, {from_address, to_address}));

    EXPECT_EQ(state.from, from_address);
    EXPECT_EQ(state.to, to_address);
    EXPECT_EQ(state.to_send, 0);
    EXPECT_EQ(state.settling_at, 0);
    EXPECT_EQ(state.min_settling_height, 0);

    EXPECT_OUTCOME_TRUE(lanes_size, state.lanes.size());
    EXPECT_EQ(lanes_size, 0);
  }

  SignedVoucher PaymentChannelActorTest::setupUpdateChannelState() {
    setupState();

    SignedVoucher voucher;
    caller = from_address;
    voucher.channel = actor_address;
    voucher.signature_bytes = Signature{Secp256k1Signature{}}.toBytes();
    ON_CALL_3(runtime,
              verifySignatureBytes(
                  voucher.signature_bytes.get(), testing::_, testing::_),
              fc::outcome::success(true));
    ON_CALL_3(runtime,
              verifySignatureBytes(testing::Not(voucher.signature_bytes.get()),
                                   testing::_,
                                   testing::_),
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
    voucher.signature_bytes = boost::none;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UpdateChannelState::call(runtime, {voucher, {}}));
  }

  /// PaymentChannelActor UpdateChannelState error: invalid voucher signature
  TEST_F(PaymentChannelActorTest, UpdateChannelStateSignatureNotVerified) {
    auto voucher = setupUpdateChannelState();
    voucher.signature_bytes = Signature{Secp256k1Signature{1}}.toBytes();

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UpdateChannelState::call(runtime, {voucher, {}}));
  }

  /// PaymentChannelActor UpdateChannelState error: epoch before voucher min
  TEST_F(PaymentChannelActorTest, UpdateChannelStateBeforeMin) {
    auto voucher = setupUpdateChannelState();
    voucher.time_lock_min = epoch + 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UpdateChannelState::call(runtime, {voucher, {}}));
  }

  /// PaymentChannelActor UpdateChannelState error: epoch after voucher max
  TEST_F(PaymentChannelActorTest, UpdateChannelStateAfterMax) {
    auto voucher = setupUpdateChannelState();
    voucher.time_lock_max = epoch - 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UpdateChannelState::call(runtime, {voucher, {}}));
  }

  /// PaymentChannelActor UpdateChannelState error: invalid secret preimage
  TEST_F(PaymentChannelActorTest, UpdateChannelStateInvalidSecretPreimage) {
    auto voucher = setupUpdateChannelState();
    voucher.secret_preimage = Buffer{blake2b_256("01"_unhex)};

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        UpdateChannelState::call(runtime, {voucher, Buffer{"02"_unhex}}));
  }

  /// PaymentChannelActor UpdateChannelState error: extra call failed
  TEST_F(PaymentChannelActorTest, UpdateChannelStateExtraFailed) {
    auto voucher = setupUpdateChannelState();
    voucher.extra = ModularVerificationParameter{
        state.to,
        123,
        {},
    };

    EXPECT_CALL(runtime,
                send(voucher.extra->actor,
                     voucher.extra->method,
                     voucher.extra->params,
                     TokenAmount{0}))
        .WillOnce(Return(asAbort(VMExitCode::kSysErrForbidden)));

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         UpdateChannelState::call(runtime, {voucher, {}}));
  }

  /// PaymentChannelActor UpdateChannelState error: expired voucher lane nonce
  TEST_F(PaymentChannelActorTest, UpdateChannelStateInvalidVoucherNonce) {
    const auto voucher = setupUpdateChannelState();
    EXPECT_OUTCOME_TRUE_1(
        state.lanes.set(voucher.lane, LaneState{{}, voucher.nonce + 1}));

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UpdateChannelState::call(runtime, {voucher, {}}));
  }

  /// PaymentChannelActor UpdateChannelState error: voucher merges to own lane
  TEST_F(PaymentChannelActorTest, UpdateChannelStateMergeSelf) {
    auto voucher = setupUpdateChannelState();
    voucher.merges.push_back(Merge{voucher.lane, {}});

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UpdateChannelState::call(runtime, {voucher, {}}));
  }

  /// PaymentChannelActor UpdateChannelState error: expired voucher merge lane
  /// nonce
  TEST_F(PaymentChannelActorTest, UpdateChannelStateInvalidMergeNonce) {
    auto voucher = setupUpdateChannelState();
    const uint64_t lane_id = 102;
    const LaneState lane{{}, 5};
    EXPECT_OUTCOME_TRUE_1(state.lanes.set(lane_id, lane));
    voucher.merges.push_back(Merge{lane_id, lane.nonce});

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UpdateChannelState::call(runtime, {voucher, {}}));
  }

  /// PaymentChannelActor UpdateChannelState error: "to send" is negative
  TEST_F(PaymentChannelActorTest, UpdateChannelStateNegative) {
    const auto voucher = setupUpdateChannelState();
    state.to_send = 10;
    EXPECT_OUTCOME_TRUE_1(state.lanes.set(
        voucher.lane,
        LaneState{voucher.amount + state.to_send + 1, voucher.nonce - 1}));

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UpdateChannelState::call(runtime, {voucher, {}}));
  }

  /// PaymentChannelActor UpdateChannelState error: "to send" exceeds balance
  TEST_F(PaymentChannelActorTest, UpdateChannelStateAboveBalance) {
    const auto voucher = setupUpdateChannelState();
    state.to_send = 10;
    balance = state.to_send + voucher.amount - 1;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         UpdateChannelState::call(runtime, {voucher, {}}));
  }

  /// PaymentChannelActor UpdateChannelState success
  TEST_F(PaymentChannelActorTest, UpdateChannelState) {
    const auto voucher = setupUpdateChannelState();
    const auto to_send = 10;
    state.to_send = to_send;
    balance = state.to_send + voucher.amount;

    EXPECT_OUTCOME_TRUE_1(UpdateChannelState::call(runtime, {voucher, {}}));

    EXPECT_EQ(state.to_send, to_send + voucher.amount);
  }

  /// PaymentChannelActor UpdateChannelState success: settling round up to min
  TEST_F(PaymentChannelActorTest, UpdateChannelStateMinHeight) {
    auto voucher = setupUpdateChannelState();
    voucher.min_close_height = 10000;
    state.settling_at = epoch + 1;
    balance = voucher.amount;

    EXPECT_OUTCOME_TRUE_1(UpdateChannelState::call(runtime, {voucher, {}}));

    EXPECT_EQ(state.settling_at, voucher.min_close_height);
    EXPECT_EQ(state.min_settling_height, voucher.min_close_height);
  }

  /// PaymentChannelActor UpdateChannelState success: voucher with merge
  TEST_F(PaymentChannelActorTest, UpdateChannelStateMerge) {
    auto voucher = setupUpdateChannelState();
    const uint64_t lane_id = 102;
    const LaneState lane{{}, 5};
    EXPECT_OUTCOME_TRUE_1(state.lanes.set(lane_id, lane));
    voucher.merges.push_back(Merge{lane_id, lane.nonce + 1});
    const auto to_send = voucher.amount - lane.redeem;
    balance = to_send;

    EXPECT_OUTCOME_TRUE_1(UpdateChannelState::call(runtime, {voucher, {}}));

    EXPECT_EQ(state.to_send, to_send);

    EXPECT_OUTCOME_TRUE(state_lane, state.lanes.get(voucher.lane));
    EXPECT_EQ(state_lane.redeem, voucher.amount);
  }

  /// PaymentChannelActor Settle error: caller not in channel
  TEST_F(PaymentChannelActorTest, SettleCallerNotInChannel) {
    setupState();
    caller = kInitAddress;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         Settle::call(runtime, {}));
  }

  /// PaymentChannelActor Settle error: caller not in channel
  TEST_F(PaymentChannelActorTest, SettleWrongSettlingAt) {
    setupState();
    caller = from_address;
    state.settling_at = epoch;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalState),
                         Settle::call(runtime, {}));
  }

  /// PaymentChannelActor Settle success: epoch round up to min
  TEST_F(PaymentChannelActorTest, SettleBelowMin) {
    setupState();
    state.min_settling_height = epoch + kSettleDelay + 1;
    caller = from_address;

    EXPECT_OUTCOME_TRUE_1(Settle::call(runtime, {}));

    EXPECT_EQ(state.settling_at, state.min_settling_height);
  }

  /// PaymentChannelActor Settle success
  TEST_F(PaymentChannelActorTest, Settle) {
    setupState();
    state.min_settling_height = epoch + kSettleDelay - 1;
    caller = from_address;

    EXPECT_OUTCOME_TRUE_1(Settle::call(runtime, {}));

    EXPECT_EQ(state.settling_at, epoch + kSettleDelay);
  }

  /// PaymentChannelActor Collect error: not settling
  TEST_F(PaymentChannelActorTest, CollectNotSettling) {
    setupState();
    caller = from_address;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         Collect::call(runtime, {}));
  }

  /// PaymentChannelActor Collect error: epoch before settled
  TEST_F(PaymentChannelActorTest, CollectBeforeSettled) {
    setupState();
    state.settling_at = epoch + 1;
    caller = from_address;

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         Collect::call(runtime, {}));
  }

  /// PaymentChannelActor Collect success
  TEST_F(PaymentChannelActorTest, Collect) {
    setupState();
    state.settling_at = epoch;
    state.to_send = 150;
    balance = 200;
    caller = from_address;
    expectSendFunds(to_address, state.to_send);

    EXPECT_CALL(runtime, deleteActor(state.from))
        .Times(testing::AtMost(1))
        .WillOnce(testing::Invoke(
            [&](auto &address) { return fc::outcome::success(); }));

    EXPECT_OUTCOME_TRUE_1(Collect::call(runtime, {}));

    EXPECT_EQ(balance, 50);
  }

}  // namespace fc::vm::actor::builtin::v2::payment_channel
