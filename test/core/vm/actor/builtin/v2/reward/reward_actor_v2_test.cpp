/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/reward/reward_actor.hpp"
#include "vm/actor/builtin/states/reward/v2/reward_actor_state.hpp"

#include <gtest/gtest.h>
#include "testutil/vm/actor/builtin/reward/reward_actor_test_fixture.hpp"
#include "vm/actor/builtin/types/reward/policy.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v2::reward {
  using primitives::ChainEpoch;
  using primitives::SpaceTime;
  using testing::Eq;
  using testing::Return;
  using testutil::vm::actor::builtin::reward::kEpochZeroReward;
  using testutil::vm::actor::builtin::reward::RewardActorTestFixture;
  using namespace types::reward;

  /**
   * Fixture with state of Reward Actor v2
   */
  struct RewardActorV2Test : public RewardActorTestFixture<RewardActorState> {
    void SetUp() override {
      RewardActorTestFixture<RewardActorState>::SetUp();
      actor_version = ActorVersion::kVersion2;
      ipld->actor_version = actor_version;
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
      EXPECT_CALL(runtime, tryResolveAddress(Eq(winner)))
          .WillOnce(Return(outcome::success(miner)));

      runtime.expectSendM<miner::ApplyRewards>(
          miner,
          {expected_reward, kPenaltyMultiplier * penalty},
          expected_reward,
          {});

      EXPECT_OUTCOME_TRUE_1(
          AwardBlockReward::call(runtime, {winner, penalty, gas_reward, 1}));
    }
  };

  /**
   * @given runtime
   * @when construct reward actor with 0 current realized power
   * @then state is equal to lotus ones
   */
  TEST_F(RewardActorV2Test, Construct0Power) {
    callerIs(kSystemActorAddress);
    const TokenAmount start_realized_power{0};

    EXPECT_OUTCOME_TRUE_1(Constructor::call(runtime, start_realized_power));

    // from Lotus
    EXPECT_EQ(SpaceTime{0}, state.cumsum_baseline);
    EXPECT_EQ(SpaceTime{0}, state.cumsum_realized);
    EXPECT_EQ(ChainEpoch{0}, state.effective_network_time);
    EXPECT_EQ(kBaselineInitialValueV2, state.effective_baseline_power);
    EXPECT_EQ(kEpochZeroReward, state.this_epoch_reward);
    EXPECT_EQ(kInitialRewardPositionEstimate,
              state.this_epoch_reward_smoothed.position);
    EXPECT_EQ(kInitialRewardVelocityEstimate,
              state.this_epoch_reward_smoothed.velocity);
    // account for rounding error of one byte during construction
    const auto epoch_zero_baseline = kBaselineInitialValueV2 - 1;
    EXPECT_EQ(epoch_zero_baseline, state.this_epoch_baseline_power);
    EXPECT_EQ(ChainEpoch{0}, state.epoch);
    EXPECT_EQ(TokenAmount{0}, state.total_reward);
    EXPECT_EQ(kSimpleTotal, state.simple_total);
    EXPECT_EQ(kBaselineTotal, state.baseline_total);
  }

  /**
   * @given runtime
   * @when construct reward actor power less than baseline
   * @then state is equal to lotus ones
   */
  TEST_F(RewardActorV2Test, ConstructPowerLessBaseline) {
    callerIs(kSystemActorAddress);
    const TokenAmount start_realized_power = BigInt{1} << 39;

    EXPECT_OUTCOME_TRUE_1(Constructor::call(runtime, start_realized_power));

    // from Lotus
    EXPECT_EQ(SpaceTime{"2888890784895207676"}, state.cumsum_baseline);
    EXPECT_EQ(start_realized_power, state.cumsum_realized);
    EXPECT_EQ(ChainEpoch{1}, state.effective_network_time);
    EXPECT_EQ(StoragePower{"2888890784895207676"},
              state.effective_baseline_power);
    EXPECT_EQ(TokenAmount{"36266280397203470018"}, state.this_epoch_reward);
    EXPECT_EQ(kInitialRewardPositionEstimate,
              state.this_epoch_reward_smoothed.position);
    EXPECT_EQ(kInitialRewardVelocityEstimate,
              state.this_epoch_reward_smoothed.velocity);
    // account for rounding error of one byte during construction
    const auto epoch_zero_baseline = kBaselineInitialValueV2 - 1;
    EXPECT_EQ(epoch_zero_baseline, state.this_epoch_baseline_power);
    EXPECT_EQ(ChainEpoch{0}, state.epoch);
    EXPECT_EQ(TokenAmount{0}, state.total_reward);
    EXPECT_EQ(kSimpleTotal, state.simple_total);
    EXPECT_EQ(kBaselineTotal, state.baseline_total);
  }

  /**
   * @given runtime
   * @when construct reward actor power more than baseline
   * @then state is equal to lotus ones
   */
  TEST_F(RewardActorV2Test, ConstructPowerMoreBaseline) {
    callerIs(kSystemActorAddress);

    const TokenAmount start_realized_power_1 = kBaselineInitialValueV2;
    EXPECT_OUTCOME_TRUE_1(Constructor::call(runtime, start_realized_power_1));

    const TokenAmount reward = state.this_epoch_reward;

    // start with 2x power
    const TokenAmount start_realized_power_2 = kBaselineInitialValueV2 * 2;
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
  TEST_F(RewardActorV2Test, RewardExceedsBalance) {
    constructRewardActor<Constructor>();
    const TokenAmount balance{9};
    setCurrentBalance(balance);
    const Address winner = Address::makeFromId(1000);
    const TokenAmount gas_reward{10};
    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalState),
        AwardBlockReward::call(runtime, {winner, 0, gas_reward, 1}));
  }

  /**
   * @given reward actor
   * @when AwardBlockReward called with penalty < 0
   * @then vm aborted with kErrIllegalArgument
   */
  TEST_F(RewardActorV2Test, RejectNegativePenalty) {
    constructRewardActor<Constructor>();
    const Address winner = Address::makeFromId(1000);
    const TokenAmount penalty{-1};
    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        AwardBlockReward::call(runtime, {winner, penalty, 0, 1}));
  }

  /**
   * @given reward actor
   * @when AwardBlockReward called with penalty < 0
   * @then vm aborted with kErrIllegalArgument
   */
  TEST_F(RewardActorV2Test, RejectNegativeReward) {
    constructRewardActor<Constructor>();
    const Address winner = Address::makeFromId(1000);
    const TokenAmount gas_reward{-1};
    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        AwardBlockReward::call(runtime, {winner, 0, gas_reward, 1}));
  }

  /**
   * @given reward actor
   * @when AwardBlockReward called with win count == 0
   * @then vm aborted with kErrIllegalArgument
   */
  TEST_F(RewardActorV2Test, RejectZeroWinCount) {
    constructRewardActor<Constructor>();
    const Address winner = Address::makeFromId(1000);
    const TokenAmount gas_reward = 10;
    const int64_t win_count{0};
    setCurrentBalance(gas_reward + 1);
    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        AwardBlockReward::call(runtime, {winner, 0, gas_reward, win_count}));
  }

  /**
   * @given reward actor with balance
   * @when AwardBlockReward called
   * @then reward is paid off
   */
  TEST_F(RewardActorV2Test, RewardPaid) {
    constructRewardActor<Constructor>();

    const TokenAmount balance = TokenAmount(1e9) * BigInt(1e18);
    setCurrentBalance(balance);
    const TokenAmount penalty{100};
    const TokenAmount gas_reward{200};
    const TokenAmount expected_reward =
        bigdiv(kEpochZeroReward, 5) + gas_reward;

    expectAwardBlockReward(penalty, gas_reward, expected_reward);
  }

  /**
   * @given reward actor with balance B < reward
   * @when AwardBlockReward called
   * @then balance is paid off
   */
  TEST_F(RewardActorV2Test, PayOutBalanceLessReward) {
    constructRewardActor<Constructor>(StoragePower{1});

    // Total reward upon writing ~1e18, so 300 should be way less
    const TokenAmount balance{300};
    setCurrentBalance(balance);
    const TokenAmount penalty{100};
    const TokenAmount gas_reward{0};
    const TokenAmount expected_reward = balance;

    expectAwardBlockReward(penalty, gas_reward, expected_reward);
  }

  /**
   * @given reward actor
   * @when reward is paid off
   * @then total mined increased
   */
  TEST_F(RewardActorV2Test, TotalReward) {
    constructRewardActor<Constructor>(StoragePower{1});
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

    EXPECT_EQ(total_payout, state.total_reward);
  }

  /**
   * @given reward actor
   * @when AwardBlockReward called and AddLockedFund fails
   * @then reward is burnt
   */
  TEST_F(RewardActorV2Test, RewardBurnsOnSendFail) {
    constructRewardActor<Constructor>(StoragePower{1});
    const TokenAmount balance{1000};
    setCurrentBalance(balance);

    const Address winner = Address::makeFromId(1000);
    const Address miner = Address::makeFromId(1100);
    EXPECT_CALL(runtime, tryResolveAddress(Eq(winner)))
        .WillOnce(Return(outcome::success(miner)));

    const TokenAmount penalty{0};
    const TokenAmount gas_reward{0};
    const TokenAmount expected_reward{1000};
    const miner::ApplyRewards::Params params{expected_reward, penalty};
    EXPECT_OUTCOME_TRUE(encoded_params, actor::encodeActorParams(params));
    EXPECT_CALL(runtime,
                send(miner,
                     miner::ApplyRewards::Number,
                     encoded_params,
                     expected_reward))
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

  /**
   * @given reward actor
   * @when method ThisEpochReward called
   * @then actual state returned
   */
  TEST_F(RewardActorV2Test, ThisEpochReward) {
    StoragePower power = BigInt{1} << 50;
    constructRewardActor<Constructor>(power);
    EXPECT_OUTCOME_TRUE(res, ThisEpochReward::call(runtime, {}));
    EXPECT_EQ(res.this_epoch_reward_smoothed.position,
              state.this_epoch_reward_smoothed.position);
    EXPECT_EQ(res.this_epoch_reward_smoothed.velocity,
              state.this_epoch_reward_smoothed.velocity);
    EXPECT_EQ(res.this_epoch_baseline_power, state.this_epoch_baseline_power);
  }

}  // namespace fc::vm::actor::builtin::v2::reward
