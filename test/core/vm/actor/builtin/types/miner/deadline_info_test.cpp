/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline_info.hpp"

#include <gtest/gtest.h>
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {

  TEST(DeadlineInfoTest, QuantizationSpecRoundsToTheNextDeadline) {
    const ChainEpoch period_start{2};
    const ChainEpoch curr = period_start + kWPoStProvingPeriod;
    DeadlineInfo d(period_start, 10, curr);
    const auto quant = d.quant();
    EXPECT_EQ(d.nextNotElapsed().last(), quant.quantizeUp(curr));
  }

  TEST(DeadlineInfoTest, OffsetAndEpochInvariantChecking) {
    const ChainEpoch pp{1972};
    const ChainEpoch pp_three{1972 + 2880 * 3};
    const ChainEpoch pp_million{1972 + int64_t(2880) * 1000000};

    const std::vector<ChainEpoch> epochs{4, 2000, 400000, 5000000};

    for (const auto epoch : epochs) {
      const auto dline_a = newDeadlineInfoFromOffsetAndEpoch(pp, epoch);
      const auto dline_b = newDeadlineInfoFromOffsetAndEpoch(pp_three, epoch);
      const auto dline_c = newDeadlineInfoFromOffsetAndEpoch(pp_million, epoch);

      EXPECT_EQ(dline_a, dline_b);
      EXPECT_EQ(dline_b, dline_c);
    }
  }

  TEST(DeadlineInfoTest, SanityChecks) {
    const ChainEpoch offset{7};
    const ChainEpoch start{2880 * 103 + offset};
    DeadlineInfo dline;

    // epoch 2880*103 + offset we are in deadline 0,
    // pp start = 2880*103 + offset
    dline = newDeadlineInfoFromOffsetAndEpoch(offset, start);
    EXPECT_EQ(dline.index, 0);
    EXPECT_EQ(dline.period_start, start);

    // epoch 2880*103 + offset + WPoStChallengeWindow - 1 we are in deadline 0
    dline = newDeadlineInfoFromOffsetAndEpoch(
        offset, start + kWPoStChallengeWindow - 1);
    EXPECT_EQ(dline.index, 0);
    EXPECT_EQ(dline.period_start, start);

    // epoch 2880*103 + offset + WPoStChallengeWindow we are in deadline 1
    dline = newDeadlineInfoFromOffsetAndEpoch(offset,
                                              start + kWPoStChallengeWindow);
    EXPECT_EQ(dline.index, 1);
    EXPECT_EQ(dline.period_start, start);

    // epoch 2880*103 + offset + 40*WPoStChallengeWindow we are in deadline 40
    dline = newDeadlineInfoFromOffsetAndEpoch(
        offset, start + 40 * kWPoStChallengeWindow);
    EXPECT_EQ(dline.index, 40);
    EXPECT_EQ(dline.period_start, start);

    // epoch 2880*103 + offset + 40*WPoStChallengeWindow - 1
    // we are in deadline 39
    dline = newDeadlineInfoFromOffsetAndEpoch(
        offset, start + 40 * kWPoStChallengeWindow - 1);
    EXPECT_EQ(dline.index, 39);
    EXPECT_EQ(dline.period_start, start);

    // epoch 2880*103 + offset + 40*WPoStChallengeWindow + 1
    // we are in deadline 40
    dline = newDeadlineInfoFromOffsetAndEpoch(
        offset, start + 40 * kWPoStChallengeWindow + 1);
    EXPECT_EQ(dline.index, 40);
    EXPECT_EQ(dline.period_start, start);

    // epoch 2880*103 + offset + WPoStPeriodDeadlines*WPoStChallengeWindow -1
    // we are in deadline WPoStPeriodDeadlines - 1
    dline = newDeadlineInfoFromOffsetAndEpoch(
        offset, start + kWPoStPeriodDeadlines * kWPoStChallengeWindow - 1);
    EXPECT_EQ(dline.index, kWPoStPeriodDeadlines - 1);
    EXPECT_EQ(dline.period_start, start);

    // epoch 2880*103 + offset + WPoStPeriodDeadlines*WPoStChallengeWindow + 1
    // we are in deadline 0, pp start = 2880*104 + offset
    dline = newDeadlineInfoFromOffsetAndEpoch(
        offset, start + kWPoStPeriodDeadlines * kWPoStChallengeWindow);
    EXPECT_EQ(dline.index, 0);
    EXPECT_EQ(dline.period_start, start + kWPoStProvingPeriod);
  }

}  // namespace fc::vm::actor::builtin::types::miner