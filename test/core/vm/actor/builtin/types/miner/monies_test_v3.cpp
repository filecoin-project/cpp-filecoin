/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/v3/monies.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  struct MoniesTestv3 : testing::Test {
    void SetUp() override {
      epoch_target_reward = TokenAmount{BigInt(1) << 50};
      sector_power = StoragePower{BigInt(1) << 36};
      network_qa_power = StoragePower{BigInt(1) << 50};

      reward_estimate = {.position = epoch_target_reward << 128,
                         .velocity = BigInt(1) << 128};
      power_estimate = {.position = network_qa_power << 128,
                        .velocity = BigInt(1) << 128};

      big_initial_pledge_factor = 20;

      EXPECT_OUTCOME_TRUE(maybe_penalty,
                          moniesv3.pledgePenaltyForTerminationLowerBound(
                              reward_estimate, power_estimate, sector_power))
      undeclared_penalty = maybe_penalty;
      big_lifetime_cap = moniesv3.termination_lifetime_cap;
    }

    Monies moniesv3;

    TokenAmount epoch_target_reward;
    StoragePower sector_power;
    StoragePower network_qa_power;

    FilterEstimate reward_estimate;
    FilterEstimate power_estimate;

    TokenAmount undeclared_penalty;
    BigInt big_initial_pledge_factor;
    BigInt big_lifetime_cap;
  };

  TEST_F(MoniesTestv3, TestPledgePenaltyForTermination) {
    const TokenAmount initial_pledge = TokenAmount{1 << 10};
    const TokenAmount day_reward =
        bigdiv(initial_pledge, big_initial_pledge_factor);
    const TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    const ChainEpoch sector_age = 20 * ChainEpoch(kEpochsInDay);

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv3.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             sector_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             sector_power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             0,
                                                             0))
    EXPECT_EQ(undeclared_penalty, fee);
  }

  TEST_F(MoniesTestv3, ExpectedReward) {
    const TokenAmount initial_pledge = undeclared_penalty;
    const TokenAmount day_reward =
        bigdiv(initial_pledge, big_initial_pledge_factor);
    const TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    const int64_t sector_age_in_days = 20;
    const ChainEpoch sector_age =
        ChainEpoch{static_cast<ChainEpoch>(sector_age_in_days * kEpochsInDay)};

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv3.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             sector_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             sector_power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             0,
                                                             0))

    const TokenAmount expected_fee =
        initial_pledge
        + bigdiv((initial_pledge * sector_age_in_days
                  * moniesv3.termination_reward_factor.numerator),
                 (big_initial_pledge_factor
                  * moniesv3.termination_reward_factor.denominator));
    EXPECT_EQ(expected_fee, fee);
  }

  TEST_F(MoniesTestv3, CappedSectorAge) {
    const TokenAmount initial_pledge = undeclared_penalty;
    const TokenAmount day_reward =
        bigdiv(initial_pledge, big_initial_pledge_factor);
    const TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    const ChainEpoch sector_age =
        ChainEpoch{static_cast<ChainEpoch>(500 * kEpochsInDay)};

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv3.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             sector_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             sector_power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             0,
                                                             0))

    const TokenAmount expected_fee =
        initial_pledge
        + bigdiv((initial_pledge * big_lifetime_cap
                  * moniesv3.termination_reward_factor.numerator),
                 (big_initial_pledge_factor
                  * moniesv3.termination_reward_factor.denominator));
    EXPECT_EQ(expected_fee, fee);
  }

  TEST_F(MoniesTestv3, EqualFeeForReplacementAndOriginal) {
    const TokenAmount initial_pledge = undeclared_penalty;
    const TokenAmount day_reward =
        bigdiv(initial_pledge, big_initial_pledge_factor);
    const TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    const ChainEpoch sector_age =
        ChainEpoch{static_cast<ChainEpoch>(20 * kEpochsInDay)};
    const ChainEpoch replacement_age =
        ChainEpoch{static_cast<ChainEpoch>(2 * kEpochsInDay)};

    const BigInt power = 1;

    EXPECT_OUTCOME_TRUE(unreplaced_fee,
                        moniesv3.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             sector_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             sector_power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             0,
                                                             0))

    EXPECT_OUTCOME_TRUE(
        actual_fee,
        moniesv3.pledgePenaltyForTermination(TokenAmount{},
                                             twenty_day_reward,
                                             replacement_age,
                                             power_estimate,
                                             reward_estimate,
                                             sector_power,
                                             NetworkVersion{},
                                             day_reward,
                                             day_reward,
                                             sector_age - replacement_age))

    EXPECT_EQ(unreplaced_fee, actual_fee);
  }

  TEST_F(MoniesTestv3, EqualFeeForReplacemntAndWithoutReplacment) {
    const TokenAmount initial_pledge = undeclared_penalty;
    const TokenAmount day_reward =
        bigdiv(initial_pledge, big_initial_pledge_factor);
    const TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    const ChainEpoch sector_age =
        ChainEpoch{static_cast<ChainEpoch>(20 * kEpochsInDay)};
    const ChainEpoch replacement_age =
        ChainEpoch{moniesv3.termination_lifetime_cap + 1} * kEpochsInDay;

    const BigInt power = 1;

    EXPECT_OUTCOME_TRUE(no_replace,
                        moniesv3.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             replacement_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             0,
                                                             0))

    EXPECT_OUTCOME_TRUE(with_replace,
                        moniesv3.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             replacement_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             day_reward,
                                                             sector_age))

    EXPECT_EQ(no_replace, with_replace);
  }

  TEST_F(MoniesTestv3, ChargerForReplacedSector) {
    const TokenAmount initial_pledge = undeclared_penalty;
    const TokenAmount day_reward =
        bigdiv(initial_pledge, big_initial_pledge_factor);
    const TokenAmount old_day_reward = 2 * day_reward;
    const TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;

    const int64_t old_sector_age_in_days = 20;
    const ChainEpoch old_sector_age = ChainEpoch{
        static_cast<ChainEpoch>(old_sector_age_in_days * kEpochsInDay)};

    const int64_t replacement_age_in_days = 15;
    const ChainEpoch replacement_age = ChainEpoch{
        static_cast<ChainEpoch>(replacement_age_in_days * kEpochsInDay)};

    const BigInt power = 1;

    const BigInt old_penalty =
        bigdiv(old_day_reward * old_sector_age_in_days
                   * moniesv3.termination_reward_factor.numerator,
               moniesv3.termination_reward_factor.denominator);
    const BigInt new_penalty =
        bigdiv(day_reward * replacement_age_in_days
                   * moniesv3.termination_reward_factor.numerator,
               moniesv3.termination_reward_factor.denominator);

    const TokenAmount expected_fee =
        twenty_day_reward + old_penalty + new_penalty;

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv3.pledgePenaltyForTermination(TokenAmount{},
                                                             twenty_day_reward,
                                                             replacement_age,
                                                             power_estimate,
                                                             reward_estimate,
                                                             power,
                                                             NetworkVersion{},
                                                             day_reward,
                                                             old_day_reward,
                                                             old_sector_age))

    EXPECT_EQ(expected_fee, fee);
  }

  TEST_F(MoniesTestv3, TestNegativeBRClamp) {
    const TokenAmount epoch_target_reward = TokenAmount{BigInt(1) << 50};
    const StoragePower sector_power = StoragePower{BigInt(1) << 36};
    const StoragePower network_qa_power = StoragePower{BigInt(1) << 10};
    const StoragePower power_rate_of_change = StoragePower{-(1 << 10)};
    const FilterEstimate reward_estimate = {.position = epoch_target_reward,
                                            .velocity = BigInt(0) << 128};
    const FilterEstimate power_estimate = {.position = network_qa_power,
                                           .velocity = power_rate_of_change};

    EXPECT_OUTCOME_TRUE(
        four_br,
        moniesv3.expectedRewardForPower(
            reward_estimate, power_estimate, sector_power, ChainEpoch(4)))
    EXPECT_EQ(0, four_br);
  }
}  // namespace fc::vm::actor::builtin::v3::miner
