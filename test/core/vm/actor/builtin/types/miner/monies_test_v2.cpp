/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/v2/monies.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  struct MoniesTestv2 : testing::Test {
    void SetUp() override {
      epoch_target_reward = TokenAmount{BigInt(1) << 50};
      sector_power = StoragePower{BigInt(1) << 36};
      network_qa_power = StoragePower{BigInt(1) << 50};
      reward_estimate = {.position = epoch_target_reward << 128,
                         .velocity = BigInt(1) << 128};
      power_estimate = {.position = network_qa_power << 128,
                        .velocity = BigInt(1) << 128};
      EXPECT_OUTCOME_TRUE(maybe_penalty,
                          moniesv2.pledgePenaltyForTerminationLowerBound(
                              reward_estimate, power_estimate, sector_power));
      undeclared_penalty = maybe_penalty;
      initial_pledge = undeclared_penalty;
      big_initial_pledge_factor = 20;
      day_reward = bigdiv(initial_pledge, big_initial_pledge_factor);
      twenty_day_reward = day_reward * big_initial_pledge_factor;
      big_lifetime_cap =
          BigInt{static_cast<int64_t>(moniesv2.termination_lifetime_cap)};
    }

    Monies moniesv2;

    TokenAmount epoch_target_reward;
    StoragePower sector_power;
    StoragePower network_qa_power;

    FilterEstimate reward_estimate;
    FilterEstimate power_estimate;

    TokenAmount undeclared_penalty;
    BigInt big_initial_pledge_factor;
    BigInt big_lifetime_cap;

    TokenAmount initial_pledge;
    TokenAmount day_reward;
    TokenAmount twenty_day_reward;
    ChainEpoch sector_age;
  };

  TEST_F(MoniesTestv2, TestPledgePenaltyForTermination) {
    const TokenAmount initial_pledge = TokenAmount{1 << 10};
    day_reward = bigdiv(initial_pledge, big_initial_pledge_factor);
    twenty_day_reward = day_reward * big_initial_pledge_factor;
    sector_age = 20 * ChainEpoch{static_cast<BigInt>(kEpochsInDay)};

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv2.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             sector_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             sector_power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             0,
                                                             0));

    EXPECT_EQ(undeclared_penalty, fee);
  };

  TEST_F(MoniesTestv2, ExpectedRewardFault) {
    day_reward = bigdiv(initial_pledge, big_initial_pledge_factor);
    twenty_day_reward = day_reward * big_initial_pledge_factor;
    const int64_t sector_age_in_days{20};
    sector_age = sector_age_in_days * kEpochsInDay;

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv2.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             sector_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             sector_power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             0,
                                                             0));

    const TokenAmount expected_fee =
        initial_pledge
        + bigdiv((initial_pledge * sector_age_in_days
                  * moniesv2.termination_reward_factor.numerator),
                 (big_initial_pledge_factor
                  * moniesv2.termination_reward_factor.denominator));
    EXPECT_EQ(expected_fee, fee);
  }

  TEST_F(MoniesTestv2, CappedSectorAge) {
    day_reward = bigdiv(initial_pledge, big_initial_pledge_factor);
    twenty_day_reward = day_reward * big_initial_pledge_factor;
    sector_age = 500 * kEpochsInDay;
    EXPECT_OUTCOME_TRUE(fee,
                        moniesv2.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             sector_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             sector_power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             0,
                                                             0));

    const TokenAmount expected_fee =
        initial_pledge
        + bigdiv((initial_pledge * big_lifetime_cap
                  * moniesv2.termination_reward_factor.numerator),
                 (big_initial_pledge_factor
                  * moniesv2.termination_reward_factor.denominator));
    EXPECT_EQ(expected_fee, fee);
  }

  TEST_F(MoniesTestv2, FeeReplacement) {
    day_reward = bigdiv(initial_pledge, big_initial_pledge_factor);
    twenty_day_reward = day_reward * big_initial_pledge_factor;
    sector_age = 20 * ChainEpoch{static_cast<BigInt>(kEpochsInDay)};
    const ChainEpoch replacement_age = 2 * kEpochsInDay;

    const BigInt power = 1;

    EXPECT_OUTCOME_TRUE(unreplaced_fee,
                        moniesv2.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             sector_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             sector_power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             0,
                                                             0));

    EXPECT_OUTCOME_TRUE(
        actual_fee,
        moniesv2.pledgePenaltyForTermination(TokenAmount{},
                                             twenty_day_reward,
                                             replacement_age,
                                             power_estimate,
                                             reward_estimate,
                                             power,
                                             NetworkVersion{},
                                             day_reward,
                                             day_reward,
                                             sector_age - replacement_age));
    EXPECT_EQ(unreplaced_fee, actual_fee);
  }

  TEST_F(MoniesTestv2, LifetimeCapReplacement) {
    day_reward = bigdiv(initial_pledge, big_initial_pledge_factor);
    twenty_day_reward = day_reward * big_initial_pledge_factor;
    sector_age = 20 * ChainEpoch{static_cast<BigInt>(kEpochsInDay)};
    const ChainEpoch replacement_age =
        (moniesv2.termination_lifetime_cap + 1) * kEpochsInDay;

    const BigInt power = 1;
    EXPECT_OUTCOME_TRUE(no_replace,
                        moniesv2.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             replacement_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             0,
                                                             0));

    EXPECT_OUTCOME_TRUE(with_replace,
                        moniesv2.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             replacement_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             day_reward,
                                                             sector_age));

    EXPECT_EQ(no_replace, with_replace);
  }

  TEST_F(MoniesTestv2, DayRateCharger) {
    day_reward = bigdiv(initial_pledge, big_initial_pledge_factor);
    twenty_day_reward = day_reward * big_initial_pledge_factor;
    const TokenAmount old_day_reward = day_reward * 2;
    const int64_t old_sector_age_in_days{20};
    const ChainEpoch old_sector_age = old_sector_age_in_days * kEpochsInDay;
    const int64_t replacement_age_in_days{15};
    const ChainEpoch replacement_age = replacement_age_in_days * kEpochsInDay;

    const BigInt power = 1;

    const BigInt old_penalty =
        bigdiv((old_day_reward * old_sector_age_in_days
                * moniesv2.termination_reward_factor.numerator),
               moniesv2.termination_reward_factor.denominator);

    const BigInt new_penalty =
        bigdiv((day_reward * replacement_age_in_days
                * moniesv2.termination_reward_factor.numerator),
               moniesv2.termination_reward_factor.denominator);

    const TokenAmount expectedFee =
        twenty_day_reward + old_penalty + new_penalty;

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv2.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             replacement_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             old_day_reward,
                                                             old_sector_age));

    EXPECT_EQ(expectedFee, fee);
  }

  TEST_F(MoniesTestv2, ExpectedRewardForPower) {
    epoch_target_reward = BigInt(1) << 50;
    sector_power = BigInt(1) << 36;
    network_qa_power = StoragePower{1 << 10};
    const StoragePower power_rate_of_change{-BigInt((1 << 10))};

    reward_estimate = {.position = epoch_target_reward << 128,
                       .velocity = BigInt(0) << 128};
    power_estimate = {.position = network_qa_power << 128,
                      .velocity = power_rate_of_change << 128};
    EXPECT_OUTCOME_TRUE(
        four_br,
        moniesv2.expectedRewardForPower(
            reward_estimate, power_estimate, sector_power, ChainEpoch(4)));

    EXPECT_EQ(four_br, 0);
  }
}  // namespace fc::vm::actor::builtin::v2::miner
