#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/v3/monies.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  struct MoniesTestv3 : testing::Test {
    virtual void SetUp() {
      epoch_target_reward = TokenAmount{1 << 50};
      qa_sector_power = StoragePower{1 << 36};
      network_qa_power = StoragePower{1 << 50};

      reward_estimate = {.position = epoch_target_reward};
      power_estimate = {.position = network_qa_power};

      big_initial_pledge_factor  = 20;

      EXPECT_OUTCOME_TRUE(maybe_penalty,
                          moniesv3.PledgePenaltyForTerminationLowerBound(
                              reward_estimate, power_estimate, qa_sector_power))
      undeclared_penalty = maybe_penalty;
      big_lifetime_cap = moniesv3.termination_lifetime_cap;
    }

    Monies moniesv3;

    TokenAmount epoch_target_reward;
    StoragePower qa_sector_power;
    StoragePower network_qa_power;

    FilterEstimate reward_estimate;
    FilterEstimate power_estimate;

    TokenAmount undeclared_penalty;
    BigInt big_initial_pledge_factor;
    BigInt big_lifetime_cap;
  };

  TEST_F(MoniesTestv3, TestPledgePenaltyForTermination) {
    TokenAmount initial_pledge = TokenAmount{1 << 10};
    TokenAmount day_reward = initial_pledge / big_initial_pledge_factor;
    TokenAmount twenty_day_reward = day_reward * big_initial_pledge_factor;
    ChainEpoch sector_age = 20 * ChainEpoch(kEpochsInDay);

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv3.PledgePenaltyForTermination(day_reward,
                                                             sector_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             qa_sector_power,
                                                             reward_estimate,
                                                             0,
                                                             0))
    ASSERT_EQ(undeclared_penalty, fee);
  }

  TEST_F(MoniesTestv3, ExpectedReward) {
    TokenAmount initial_pledge = undeclared_penalty;
    TokenAmount day_reward = initial_pledge / big_initial_pledge_factor;
    TokenAmount twenty_day_reward = day_reward * big_initial_pledge_factor;
    int64_t sector_age_in_days = 20;
    ChainEpoch sector_age =
        ChainEpoch{static_cast<ChainEpoch>(sector_age_in_days * kEpochsInDay)};

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv3.PledgePenaltyForTermination(day_reward,
                                                             sector_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             qa_sector_power,
                                                             reward_estimate,
                                                             0,
                                                             0))

    TokenAmount expected_fee =
        initial_pledge
        + (initial_pledge * sector_age_in_days
           * moniesv3.termination_reward_factor.numerator)
              / (big_initial_pledge_factor
                 * moniesv3.termination_reward_factor.denominator);
    ASSERT_EQ(expected_fee, fee);
  }

  TEST_F(MoniesTestv3, CappedSectorAge) {
    TokenAmount initial_pledge = undeclared_penalty;
    TokenAmount day_reward = initial_pledge / big_initial_pledge_factor;
    TokenAmount twenty_day_reward = day_reward * big_initial_pledge_factor;
    ChainEpoch sector_age =
        ChainEpoch{static_cast<ChainEpoch>(500 * kEpochsInDay)};

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv3.PledgePenaltyForTermination(day_reward,
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
           * moniesv3.termination_reward_factor.numerator)
              / (big_initial_pledge_factor
                 * moniesv3.termination_reward_factor.denominator);
    ASSERT_EQ(expected_fee, fee);
  }

  TEST_F(MoniesTestv3, EqualFeeForReplacementAndOriginal) {
    TokenAmount initial_pledge = undeclared_penalty;
    TokenAmount day_reward = initial_pledge / big_initial_pledge_factor;
    TokenAmount twenty_day_reward = day_reward * big_initial_pledge_factor;
    ChainEpoch sector_age =
        ChainEpoch{static_cast<ChainEpoch>(20 * kEpochsInDay)};
    ChainEpoch replacement_age =
        ChainEpoch{static_cast<ChainEpoch>(2 * kEpochsInDay)};

    BigInt power = 1;

    EXPECT_OUTCOME_TRUE(unreplaced_fee,
                        moniesv3.PledgePenaltyForTermination(day_reward,
                                                             sector_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             power,
                                                             reward_estimate,
                                                             0,
                                                             0))

    EXPECT_OUTCOME_TRUE(
        actual_fee,
        moniesv3.PledgePenaltyForTermination(day_reward,
                                             replacement_age,
                                             twenty_day_reward,
                                             power_estimate,
                                             power,
                                             reward_estimate,
                                             day_reward,
                                             sector_age - replacement_age))

    ASSERT_EQ(unreplaced_fee, actual_fee);
  }

  TEST_F(MoniesTestv3, EqualFeeForReplacemntAndWithoutReplacment) {
    TokenAmount initial_pledge = undeclared_penalty;
    TokenAmount day_reward = initial_pledge / big_initial_pledge_factor;
    TokenAmount twenty_day_reward = day_reward * big_initial_pledge_factor;
    ChainEpoch sector_age =
        ChainEpoch{static_cast<ChainEpoch>(20 * kEpochsInDay)};
    ChainEpoch replacement_age =
        ChainEpoch{moniesv3.termination_lifetime_cap + 1} * kEpochsInDay;

    BigInt power = 1;

    EXPECT_OUTCOME_TRUE(no_replace,
                        moniesv3.PledgePenaltyForTermination(day_reward,
                                                             replacement_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             power,
                                                             reward_estimate,
                                                             0,
                                                             0))

    EXPECT_OUTCOME_TRUE(with_replace,
                        moniesv3.PledgePenaltyForTermination(day_reward,
                                                             replacement_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             power,
                                                             reward_estimate,
                                                             day_reward,
                                                             sector_age))

    ASSERT_EQ(no_replace, with_replace);
  }

  TEST_F(MoniesTestv3, ChargerForReplacedSector) {
    TokenAmount initial_pledge = undeclared_penalty;
    TokenAmount day_reward = initial_pledge / big_initial_pledge_factor;
    TokenAmount old_day_reward = 2 * day_reward;
    TokenAmount twenty_day_reward = day_reward * big_initial_pledge_factor;

    int64_t old_sector_age_in_days = 20;
    ChainEpoch old_sector_age = ChainEpoch{
        static_cast<ChainEpoch>(old_sector_age_in_days * kEpochsInDay)};

    int64_t replacement_age_in_days = 15;
    ChainEpoch replacement_age = ChainEpoch{
        static_cast<ChainEpoch>(replacement_age_in_days * kEpochsInDay)};

    BigInt power = 1;

    BigInt old_penalty = old_day_reward * old_sector_age_in_days
                         * moniesv3.termination_reward_factor.numerator
                         / moniesv3.termination_reward_factor.denominator;
    BigInt new_penalty = day_reward * replacement_age_in_days
                         * moniesv3.termination_reward_factor.numerator
                         / moniesv3.termination_reward_factor.denominator;

    TokenAmount expected_fee = twenty_day_reward + old_penalty + new_penalty;

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv3.PledgePenaltyForTermination(day_reward,
                                                             replacement_age,
                                                             twenty_day_reward,
                                                             power_estimate,
                                                             power,
                                                             reward_estimate,
                                                             old_day_reward,
                                                             old_sector_age))

    ASSERT_EQ(expected_fee, fee);
  }

  TEST_F(MoniesTestv3, TestNegativeBRClamp) {
    TokenAmount epoch_target_reward = TokenAmount{1 << 50};
    StoragePower qa_sector_power = StoragePower{1 << 36};
    StoragePower network_qa_power = StoragePower{1 << 10};
    StoragePower power_rate_of_change = StoragePower{-(1 << 10)};
    FilterEstimate reward_estimate = {.position = epoch_target_reward,
                                      .velocity = 0};
    FilterEstimate power_estimate = {.position = network_qa_power,
                                     .velocity = power_rate_of_change};

    EXPECT_OUTCOME_TRUE(
        four_br,
        moniesv3.ExpectedRewardForPower(
            reward_estimate, power_estimate, qa_sector_power, ChainEpoch(4)))
    ASSERT_EQ(0, four_br);
  }
}  // namespace fc::vm::actor::builtin::v3::miner