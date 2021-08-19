/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/v2/monies.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  struct MoniesTestv2 : testing::Test {
    virtual void SetUp() {
      epoch_target_reward = TokenAmount{1 << 50};
      qa_sector_power = StoragePower{1 << 36};
      network_qa_power = StoragePower{1 << 50};
      reward_estimate = {.position = epoch_target_reward};
      power_estimate = {.position = network_qa_power};
      EXPECT_OUTCOME_TRUE(
          maybe_penalty,
          moniesv2.PledgePenaltyForTerminationLowerBound(
              reward_estimate, power_estimate, qa_sector_power));
      undeclared_penalty = maybe_penalty;
      initial_pledge = undeclared_penalty;
      big_initial_pledge_factor = 20;
      day_reward = initial_pledge / big_initial_pledge_factor;
      twenty_day_reward = day_reward * big_initial_pledge_factor;
      big_lifetime_cap =
          BigInt{static_cast<int64_t>(moniesv2.termination_lifetime_cap)};
    }

    Monies moniesv2;

    TokenAmount epoch_target_reward;
    StoragePower qa_sector_power;
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
    TokenAmount initial_pledge = TokenAmount{1 << 10};
    sector_age = 20 * ChainEpoch{static_cast<BigInt>(kEpochsInDay)};

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv2.PledgePenaltyForTermination(day_reward,
                                                             sector_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             qa_sector_power,
                                                             reward_estimate,
                                                             0,
                                                             0));

    ASSERT_EQ(undeclared_penalty, fee);
    std::cout<<"TestPledgePenaltyForTermination:"<<undeclared_penalty<<"  "<<fee;
  };

  TEST_F(MoniesTestv2, ExpectedRewardFault) {
    int64_t sector_age_in_days{20};
    sector_age = sector_age_in_days * kEpochsInDay;

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv2.PledgePenaltyForTermination(day_reward,
                                                             sector_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             qa_sector_power,
                                                             reward_estimate,
                                                             0,
                                                             0));

    TokenAmount expected_fee =
        initial_pledge
        + (initial_pledge * sector_age_in_days
           * moniesv2.termination_reward_factor.numerator
           / big_initial_pledge_factor
           * moniesv2.termination_reward_factor.denominator);
    ASSERT_EQ(expected_fee, fee);
    std::cout<<"ExpectedRewardFault:"<<expected_fee<<"  "<<fee;

  }

  TEST_F(MoniesTestv2, CappedSectorAge) {
    sector_age = 500 * kEpochsInDay;
    EXPECT_OUTCOME_TRUE(fee,
                        moniesv2.PledgePenaltyForTermination(day_reward,
                                                             sector_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             qa_sector_power,
                                                             reward_estimate,
                                                             0,
                                                             0))

    TokenAmount expected_fee =
        initial_pledge
        + (initial_pledge * big_lifetime_cap
           * moniesv2.termination_reward_factor.numerator)
              / (big_initial_pledge_factor
                 * moniesv2.termination_reward_factor.denominator);
    ASSERT_EQ(expected_fee, fee);
    std::cout<<"CappedSectorAge:"<<expected_fee<<"  "<<fee;

  }

  TEST_F(MoniesTestv2, FeeReplacement) {
    sector_age = 20 * ChainEpoch{static_cast<BigInt>(kEpochsInDay)};
    ChainEpoch replacement_age = 2 * kEpochsInDay;

    BigInt power = 1;

    EXPECT_OUTCOME_TRUE(unreplaced_fee,
                        moniesv2.PledgePenaltyForTermination(day_reward,
                                                             sector_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             qa_sector_power,
                                                             reward_estimate,
                                                             0,
                                                             0));

    EXPECT_OUTCOME_TRUE(
        actual_fee,
        moniesv2.PledgePenaltyForTermination(day_reward,
                                             replacement_age,
                                             twenty_day_reward,
                                             power_estimate,
                                             power,
                                             reward_estimate,
                                             day_reward,
                                             sector_age - replacement_age))
    ASSERT_EQ(unreplaced_fee, actual_fee);
  }

  TEST_F(MoniesTestv2, LifetimeCapReplacement) {
    sector_age = 20 * ChainEpoch{static_cast<BigInt>(kEpochsInDay)};
    ChainEpoch replacement_age =
        (moniesv2.termination_lifetime_cap + 1) * kEpochsInDay;

    BigInt power = 1;
    EXPECT_OUTCOME_TRUE(no_replace,
                        moniesv2.PledgePenaltyForTermination(day_reward,
                                                             replacement_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             power,
                                                             reward_estimate,
                                                             0,
                                                             0))

    EXPECT_OUTCOME_TRUE(with_replace,
                        moniesv2.PledgePenaltyForTermination(day_reward,
                                                             replacement_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             power,
                                                             reward_estimate,
                                                             day_reward,
                                                             sector_age))

    ASSERT_EQ(no_replace, with_replace);
  }

  TEST_F(MoniesTestv2, DayRateCharger) {
    TokenAmount old_day_reward = day_reward * 2;
    int64_t old_sector_age_in_days{20};
    ChainEpoch old_sector_age = old_sector_age_in_days * kEpochsInDay;
    int64_t replacement_age_in_days{15};
    ChainEpoch replacement_age = replacement_age_in_days * kEpochsInDay;

    BigInt power = 1;

    BigInt old_penalty = (old_day_reward * old_sector_age_in_days
                          * moniesv2.termination_reward_factor.numerator)
                         / moniesv2.termination_reward_factor.denominator;

    BigInt new_penalty = (day_reward * replacement_age_in_days
                          * moniesv2.termination_reward_factor.numerator)
                         / moniesv2.termination_reward_factor.denominator;

    TokenAmount expectedFee = twenty_day_reward + old_penalty + new_penalty;

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv2.PledgePenaltyForTermination(day_reward,
                                                             replacement_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             power,
                                                             reward_estimate,
                                                             old_day_reward,
                                                             old_sector_age))

    ASSERT_EQ(expectedFee, fee);
  }

  TEST_F(MoniesTestv2, ExpectedRewardForPower) {
    network_qa_power = StoragePower{1 << 10};
    StoragePower power_rate_of_change{-1 * (1 << 10)};
    power_estimate = {.position = network_qa_power,
                      .velocity = power_rate_of_change};

    EXPECT_OUTCOME_TRUE(
        four_br,
        moniesv2.ExpectedRewardForPower(
            reward_estimate, power_estimate, qa_sector_power, ChainEpoch{4}))

    ASSERT_FALSE(four_br);
  }
}  // namespace fc::vm::actor::builtin::v2::miner