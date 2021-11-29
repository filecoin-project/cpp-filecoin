/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/reward/reward_actor_calculus.hpp"

#include <gtest/gtest.h>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <common/math/math.hpp>
#include "const.hpp"
#include "testutil/resources/parse.hpp"
#include "testutil/resources/resources.hpp"
#include "vm/actor/builtin/types/reward/policy.hpp"

namespace fc::vm::actor::builtin::types::reward {
  using common::math::kPrecision128;
  using BigFloat = boost::multiprecision::cpp_dec_float_100;
  using Params = std::tuple<StoragePower, BigFloat>;

  struct RewardActorCalculusV0 : testing::TestWithParam<Params> {};

  /**
   * Converts Q.128 number to float
   * @param x - Q.128 format
   * @return float number
   */
  double q128ToF(const BigInt &x) {
    return (x.convert_to<BigFloat>()
            / (BigInt{1} << 128).convert_to<BigFloat>())
        .convert_to<double>();
  }

  StoragePower baselinePowerAt(const ChainEpoch &epoch) {
    return StoragePower(epoch + 1) * BigInt(2048);
  };

  StoragePower baselineInYears(const StoragePower &start,
                               const ChainEpoch &epoch) {
    auto baseline = start;
    for (ChainEpoch i = 0; i < static_cast<ChainEpoch>(epoch * kEpochsInYear);
         ++i) {
      baseline = baselinePowerFromPrev(baseline, kBaselineExponentV0);
    }
    return baseline;
  }

  /**
   * Test data from specs-actors v0.9.12
   * 'actors/builtin/reward/reward_logic_test.go'
   */
  TEST(RewardActorCalculusV0, TestComputeRTeta) {
    const ChainEpoch epoch{1};
    ASSERT_DOUBLE_EQ(
        0.5,
        q128ToF(computeRTheta(
            epoch, baselinePowerAt(epoch), SpaceTime(4096), SpaceTime(6144))));
    ASSERT_DOUBLE_EQ(
        0.25,
        q128ToF(computeRTheta(
            epoch, baselinePowerAt(epoch), SpaceTime(3072), SpaceTime(6144))));
  }

  /**
   * Test data from specs-actors v0.9.12
   * 'actors/builtin/reward/reward_logic_test.go'
   */
  TEST(RewardActorCalculusV0, TestComputeRTetaCumSum) {
    BigInt cumsum;
    const ChainEpoch epoch{16};
    for (ChainEpoch i = 0; i < epoch; ++i) {
      cumsum += baselinePowerAt(i);
    }
    ASSERT_DOUBLE_EQ(
        15.25,
        q128ToF(computeRTheta(epoch,
                              baselinePowerAt(epoch),
                              cumsum + bigdiv(baselinePowerAt(epoch), 4),
                              cumsum + baselinePowerAt(epoch))));
  }

  /**
   * Test simple reward against
   * 'specs-actors/actors/builtin/testdata/TestSimpleReward.golden'
   */
  TEST(RewardActorCalculusV0, TestSimpleReward) {
    const auto test_data = parseCsvPair(
        resourcePath("vm/actor/builtin/v0/reward/test_simple_reward.txt"));

    for (const auto &[epoch, expected_reward] : test_data) {
      ASSERT_EQ(expected_reward,
                computeReward(epoch.convert_to<ChainEpoch>(),
                              0,
                              0,
                              kSimpleTotal,
                              kBaselineTotal));
    }
  }

  /**
   * Test simple reward against
   * 'specs-actors/actors/builtin/testdata/TestBaselineReward.golden'
   */
  TEST(RewardActorCalculusV0, TestBaselineReward) {
    const auto test_data = parseCsvTriples(
        resourcePath("vm/actor/builtin/v0/reward/test_baseline_reward.txt"));
    const auto simple = computeReward(0, 0, 0, kSimpleTotal, kBaselineTotal);
    for (const auto &[prev_theta, theta, expected_reward] : test_data) {
      const TokenAmount reward =
          computeReward(0, prev_theta, theta, kSimpleTotal, kBaselineTotal)
          - simple;
      ASSERT_EQ(expected_reward, reward);
    }
  }

  /**
   * Test data from specs-actors v0.9.12
   * 'actors/builtin/reward/reward_logic_test.go'
   *
   * Baseline reward should have 200% growth rate. This implies that for every
   * year x, the baseline function should be: StartVal * 3^x. Error values for 1
   * years of growth were determined empirically with latest baseline power
   * construction to set bounds in this test in order to
   * 1. throw a test error if function changes and percent error goes up
   * 2. serve as documentation of current error bounds
   */
  TEST_P(RewardActorCalculusV0, TestBaselineRewardGrowth) {
    const auto &[start, err_bound] = GetParam();
    const auto end = baselineInYears(start, ChainEpoch{1});
    const auto expected = start * 3;
    const BigFloat err = BigFloat{expected - end} / BigFloat{expected};

    ASSERT_LT(err, err_bound);
  }

  INSTANTIATE_TEST_SUITE_P(RewardActorCalculusV0Cases,
                           RewardActorCalculusV0,
                           ::testing::Values(
                               // 1 byte
                               Params{StoragePower{1}, BigFloat{1}},
                               // GiB
                               Params{StoragePower{1} << 30, BigFloat{1e-3}},
                               // TiB
                               Params{StoragePower{1} << 40, BigFloat{1e-6}},
                               // PiB
                               Params{StoragePower{1} << 50, BigFloat{1e-8}},
                               // EiB
                               Params{kBaselineInitialValueV0, BigFloat{1e-8}},
                               // ZiB
                               Params{StoragePower{1} << 70, BigFloat{1e-8}},
                               // non power of 2 ~ 1 EiB
                               Params{StoragePower{"513633559722596517"},
                                      BigFloat{1e-8}}));

}  // namespace fc::vm::actor::builtin::types::reward
