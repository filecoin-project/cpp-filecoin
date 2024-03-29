/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/v0/monies.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  class MoniesTestv0 : public testing::Test {
   protected:
    void SetUp() override {
      nv = version::NetworkVersion::kVersion0;
      epoch_target_reward = TokenAmount{BigInt(1) << 50};
      sector_power = StoragePower{BigInt(1) << 36};
      network_qa_power = StoragePower{BigInt(1) << 50};

      reward_estimate = {.position = epoch_target_reward << 128,
                         .velocity = BigInt(0) << 128};
      power_estimate = FilterEstimate{.position = network_qa_power << 128,
                                      .velocity = BigInt(0) << 128};

      EXPECT_OUTCOME_TRUE(
          maybe_penalty,
          moniesv0.pledgePenaltyForUndeclaredFault(
              reward_estimate, power_estimate, sector_power, nv));
      undeclared_penalty_ = TokenAmount{maybe_penalty};
      big_initial_pledge_factor = 20;
    }

    Monies moniesv0;

    version::NetworkVersion nv;

    TokenAmount epoch_target_reward;
    StoragePower sector_power;
    StoragePower network_qa_power;

    FilterEstimate reward_estimate;
    FilterEstimate power_estimate;

    TokenAmount undeclared_penalty_;
    BigInt big_initial_pledge_factor;
  };

  TEST_F(MoniesTestv0, TestPledgePenaltyForTermination) {
    const TokenAmount initial_pledge = TokenAmount{1 << 10};
    const TokenAmount day_reward =
        bigdiv(initial_pledge, big_initial_pledge_factor);
    const TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    const ChainEpoch sector_age = 20 * static_cast<ChainEpoch>(kEpochsInDay);

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv0.pledgePenaltyForTermination(day_reward,
                                                             twenty_day_reward,
                                                             sector_age,
                                                             reward_estimate,
                                                             power_estimate,
                                                             sector_power,
                                                             nv,
                                                             0,
                                                             0,
                                                             0));

    EXPECT_EQ(fee, undeclared_penalty_);
  }

  TEST_F(MoniesTestv0, ExpectedRewardFault) {
    const TokenAmount initial_pledge = undeclared_penalty_;
    const TokenAmount day_reward =
        bigdiv(initial_pledge, big_initial_pledge_factor);
    const TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    const int64_t sector_age_in_days = 20;
    const ChainEpoch sector_age =
        ChainEpoch{static_cast<ChainEpoch>(sector_age_in_days * kEpochsInDay)};

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv0.pledgePenaltyForTermination(day_reward,
                                                             twenty_day_reward,
                                                             sector_age,
                                                             reward_estimate,
                                                             power_estimate,
                                                             sector_power,
                                                             nv,
                                                             0,
                                                             0,
                                                             0));

    const TokenAmount expected_fee =
        TokenAmount{initial_pledge
                    + bigdiv(initial_pledge * sector_age_in_days,
                             big_initial_pledge_factor)};
    EXPECT_EQ(fee, expected_fee);
  }

  TEST_F(MoniesTestv0, CappedSectorAge) {
    const TokenAmount initial_pledge = undeclared_penalty_;
    const TokenAmount day_reward =
        bigdiv(initial_pledge, big_initial_pledge_factor);
    const TokenAmount twenty_day_reward =
        day_reward * big_initial_pledge_factor;
    const int64_t sector_age_in_days = 500;
    const ChainEpoch sector_age =
        ChainEpoch{static_cast<ChainEpoch>(sector_age_in_days * kEpochsInDay)};

    EXPECT_OUTCOME_TRUE(fee,
                        moniesv0.pledgePenaltyForTermination(day_reward,
                                                             twenty_day_reward,
                                                             sector_age,
                                                             reward_estimate,
                                                             power_estimate,
                                                             sector_power,
                                                             nv,
                                                             0,
                                                             0,
                                                             0));

    const TokenAmount expected_fee =
        initial_pledge
        + bigdiv((initial_pledge * moniesv0.termination_lifetime_cap),
                 big_initial_pledge_factor);
    EXPECT_EQ(fee, expected_fee);
  }
}  // namespace fc::vm::actor::builtin::v0::miner
