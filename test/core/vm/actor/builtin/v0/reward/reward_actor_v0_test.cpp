/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/reward/reward_actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor_state.hpp"

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v0::reward {
  using runtime::MockRuntime;
  using storage::ipfs::InMemoryDatastore;
  using testing::Eq;
  using testing::Return;

  static const TokenAmount kEpochZeroReward{"36266264293777134739"};

  class RewardActorV0Test : public testing::Test {
    void SetUp() override {
      EXPECT_CALL(runtime, getIpfsDatastore())
          .Times(testing::AnyNumber())
          .WillRepeatedly(Return(ipld));

      EXPECT_CALL(runtime, getImmediateCaller())
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Invoke([&]() { return caller; }));

      EXPECT_CALL(runtime, getCurrentActorState())
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Invoke([&]() {
            EXPECT_OUTCOME_TRUE(cid, ipld->setCbor(state));
            return std::move(cid);
          }));

      EXPECT_CALL(runtime, commit(testing::_))
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Invoke([&](auto &cid) {
            EXPECT_OUTCOME_TRUE(new_state, ipld->getCbor<State>(cid));
            state = std::move(new_state);
            return fc::outcome::success();
          }));
    }

   public:
    void setCurrentBalance(const TokenAmount &balance) {
      const Address receiver = Address::makeFromId(1001);
      EXPECT_CALL(runtime, getCurrentReceiver())
          .WillRepeatedly(Return(receiver));
      EXPECT_CALL(runtime, getBalance(Eq(receiver)))
          .WillRepeatedly(Return(outcome::success(balance)));
    }

    void constructRewardActor(const StoragePower &start_realized_power = 0) {
      caller = kSystemActorAddress;
      EXPECT_OUTCOME_TRUE_1(Constructor::call(runtime, start_realized_power));
    }

    /**
     * Expect successful call AwardBlockReward
     * @param penalty - penalty burnt
     * @param gas_reward
     * @param expected_reward - expected reward for winner
     */
    void expectAwardBlockReward(const TokenAmount &penalty,
                                const TokenAmount &gas_reward,
                                const TokenAmount &expected_reward) {
      const Address winner = Address::makeFromId(1000);
      const Address miner = Address::makeFromId(1100);
      EXPECT_CALL(runtime, resolveAddress(Eq(winner)))
          .WillOnce(Return(outcome::success(miner)));

      runtime.expectSendM<miner::AddLockedFund>(
          miner, expected_reward, expected_reward, {});
      if (penalty > 0) {
        EXPECT_CALL(runtime,
                    send(Eq(kBurntFundsActorAddress),
                         Eq(kSendMethodNumber),
                         Eq(Buffer{}),
                         Eq(penalty)))
            .WillOnce(Return(outcome::success()));
      }

      EXPECT_OUTCOME_TRUE_1(
          AwardBlockReward::call(runtime, {winner, penalty, gas_reward, 1}));
    }

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    Address caller;
    State state;
  };

  /**
   * @given runtime
   * @when construct reward actor with 0 current realized power
   * @then state is equal to lotus ones
   */
  TEST_F(RewardActorV0Test, Construct0Power) {
    caller = kSystemActorAddress;
    const TokenAmount start_realized_power{0};

    EXPECT_OUTCOME_TRUE_1(Constructor::call(runtime, start_realized_power));
    EXPECT_EQ(ChainEpoch{0}, state.epoch);
    EXPECT_EQ(StoragePower{0}, state.cumsum_realized);
    // from Lotus
    EXPECT_EQ(kEpochZeroReward, state.this_epoch_reward);
    // account for rounding error of one byte during construction
    const auto epoch_zero_baseline = kBaselineInitialValueV0 - 1;
    EXPECT_EQ(epoch_zero_baseline, state.this_epoch_baseline_power);
    EXPECT_EQ(kBaselineInitialValueV0, state.effective_baseline_power);
  }

  /**
   * @given runtime
   * @when construct reward actor power less than baseline
   * @then state is equal to lotus ones
   */
  TEST_F(RewardActorV0Test, ConstructPowerLessBaseline) {
    caller = kSystemActorAddress;
    const TokenAmount start_realized_power = BigInt{1} << 39;

    EXPECT_OUTCOME_TRUE_1(Constructor::call(runtime, start_realized_power));
    EXPECT_EQ(ChainEpoch{0}, state.epoch);
    EXPECT_EQ(start_realized_power, state.cumsum_realized);

    // from Lotus
    EXPECT_EQ(TokenAmount{"36266304644305024178"}, state.this_epoch_reward);
    EXPECT_EQ(StoragePower{"1152921504606846975"},
              state.this_epoch_baseline_power);
    EXPECT_EQ(StoragePower{"1152922709529216365"},
              state.effective_baseline_power);
  }

  /**
   * @given runtime
   * @when construct reward actor power more than baseline
   * @then state is equal to lotus ones
   */
  TEST_F(RewardActorV0Test, ConstructPowerMoreBaseline) {
    caller = kSystemActorAddress;

    const TokenAmount start_realized_power_1 = BigInt{1} << 60;
    EXPECT_OUTCOME_TRUE_1(Constructor::call(runtime, start_realized_power_1));

    const TokenAmount reward = state.this_epoch_reward;

    // start with 2x power
    const TokenAmount start_realized_power_2 = BigInt{2} << 60;
    EXPECT_OUTCOME_TRUE_1(Constructor::call(runtime, start_realized_power_2));

    // Reward value is the same; realized power impact on reward is capped at
    // baseline
    EXPECT_EQ(reward, state.this_epoch_reward);
  }

  /**
   * @given reward actor with balance B
   * @when AwardBlockReward with reward > B is called
   * @then vm aborted with kErrIllegalState
   */
  TEST_F(RewardActorV0Test, RewardExceedsBalance) {
    constructRewardActor();
    const TokenAmount balance{9};
    setCurrentBalance(balance);

    const Address winner = Address::makeFromId(1000);
    const TokenAmount gas_reward{10};
    auto res = AwardBlockReward::call(runtime, {winner, 0, gas_reward, 1});

    EXPECT_TRUE(res.has_error());
    auto error = res.error();
    EXPECT_TRUE(isAbortExitCode(error));
    EXPECT_EQ(VMAbortExitCode{VMExitCode::kErrIllegalState}, error);
  }

  /**
   * @given reward actor
   * @when AwardBlockReward called with penalty < 0
   * @then vm aborted with kErrIllegalArgument
   */
  TEST_F(RewardActorV0Test, RejectNegativePenalty) {
    constructRewardActor();

    const Address winner = Address::makeFromId(1000);
    const TokenAmount penalty{-1};
    const auto res = AwardBlockReward::call(runtime, {winner, penalty, 0, 1});

    EXPECT_TRUE(res.has_error());
    const auto error = res.error();
    EXPECT_TRUE(isAbortExitCode(error));
    EXPECT_EQ(VMAbortExitCode{VMExitCode::kErrIllegalArgument}, error);
  }

  /**
   * @given reward actor
   * @when AwardBlockReward called with penalty < 0
   * @then vm aborted with kErrIllegalArgument
   */
  TEST_F(RewardActorV0Test, RejectNegativeReward) {
    constructRewardActor();

    const Address winner = Address::makeFromId(1000);
    const TokenAmount gas_reward{-1};
    const auto res = AwardBlockReward::call(runtime, {winner, 0, gas_reward, 1});

    EXPECT_TRUE(res.has_error());
    const auto error = res.error();
    EXPECT_TRUE(isAbortExitCode(error));
    EXPECT_EQ(VMAbortExitCode{VMExitCode::kErrIllegalArgument}, error);
  }

  /**
   * @given reward actor
   * @when AwardBlockReward called with win count == 0
   * @then vm aborted with kErrIllegalArgument
   */
  TEST_F(RewardActorV0Test, RejectZeroWinCount) {
    constructRewardActor();

    const Address winner = Address::makeFromId(1000);
    const int64_t win_count{0};
    const auto res = AwardBlockReward::call(runtime, {winner, 0, 0, win_count});

    EXPECT_TRUE(res.has_error());
    const auto error = res.error();
    EXPECT_TRUE(isAbortExitCode(error));
    EXPECT_EQ(VMAbortExitCode{VMExitCode::kErrIllegalArgument}, error);
  }

  /**
   * @given reward actor with balance
   * @when AwardBlockReward called
   * @then reward is payed and penalty is burnt
   */
  TEST_F(RewardActorV0Test, RewardPayedPenaltyBurnt) {
    constructRewardActor();

    const TokenAmount balance = TokenAmount(1e9) * BigInt(1e18);
    setCurrentBalance(balance);
    const TokenAmount penalty{100};
    const TokenAmount gas_reward{200};
    const TokenAmount expected_reward =
        bigdiv(kEpochZeroReward, 5) + gas_reward - penalty;

    expectAwardBlockReward(penalty, gas_reward, expected_reward);
  }

  /**
   * @given reward actor with balance B < reward
   * @when AwardBlockReward called
   * @then balance is payed off
   */
  TEST_F(RewardActorV0Test, PayOutBalanceLessReward) {
    constructRewardActor(StoragePower{1});

    // Total reward upon writing ~1e18, so 300 should be way less
    const TokenAmount balance{300};
    setCurrentBalance(balance);
    const TokenAmount penalty{100};
    const TokenAmount gas_reward{0};
    const TokenAmount expected_reward = balance - penalty;

    expectAwardBlockReward(penalty, gas_reward, expected_reward);
  }

  /**
   * @given reward actor
   * @when reward is payed off
   * @then total mined increased
   */
  TEST_F(RewardActorV0Test, TotalReward) {
    constructRewardActor(StoragePower{1});
    const TokenAmount total_payout{3500};
    TokenAmount balance = total_payout;
    setCurrentBalance(balance);

    state.this_epoch_reward = TokenAmount{5000};

    const TokenAmount penalty{0};
    const TokenAmount gas_reward{0};
    // award normalized by expected leaders is 1000
    TokenAmount expected_reward{1000};

    // enough balance to pay 3 full rewards and one partial
    expectAwardBlockReward(penalty, gas_reward, expected_reward);
    balance -= expected_reward;
    setCurrentBalance(balance);
    expectAwardBlockReward(penalty, gas_reward, expected_reward);
    balance -= expected_reward;
    setCurrentBalance(balance);
    expectAwardBlockReward(penalty, gas_reward, expected_reward);
    balance -= expected_reward;
    setCurrentBalance(balance);
    // balance (500) < reward (1000)
    expected_reward = 500;
    expectAwardBlockReward(penalty, gas_reward, expected_reward);

    EXPECT_EQ(total_payout, state.total_mined);
  }

  /**
   * @given reward actor
   * @when AwardBlockReward called and AddLockedFund fails
   * @then reward is burnt
   */
  TEST_F(RewardActorV0Test, RewardBurnsOnSendFail) {
    constructRewardActor(StoragePower{1});
    const TokenAmount balance{1000};
    setCurrentBalance(balance);

    const Address winner = Address::makeFromId(1000);
    const Address miner = Address::makeFromId(1100);
    EXPECT_CALL(runtime, resolveAddress(Eq(winner)))
        .WillOnce(Return(outcome::success(miner)));

    const TokenAmount penalty{0};
    const TokenAmount gas_reward{0};
    const TokenAmount expected_reward{1000};

    EXPECT_OUTCOME_TRUE(params, actor::encodeActorParams(expected_reward));
    EXPECT_CALL(
        runtime,
        send(miner, miner::AddLockedFund::Number, params, expected_reward))
        .WillOnce(testing::Return(VMExitCode::kErrForbidden));
    EXPECT_CALL(runtime,
                send(Eq(kBurntFundsActorAddress),
                     Eq(kSendMethodNumber),
                     Eq(Buffer{}),
                     Eq(expected_reward)))
        .WillOnce(Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(
        AwardBlockReward::call(runtime, {winner, penalty, gas_reward, 1}));
  }

}  // namespace fc::vm::actor::builtin::v0::reward
