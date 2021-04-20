/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/termination.hpp"

#include <gtest/gtest.h>

namespace fc::vm::actor::builtin::types::miner {

  /**
   * Check methods of TerminationResult
   */
  TEST(TerminationResultTest, Test) {
    TerminationResult result;
    EXPECT_EQ(result.isEmpty(), true);

    const TerminationResult resultA{
        .sectors = {{3, {9}}, {0, {1, 2, 4}}, {2, {3, 5, 7}}},
        .partitions_processed = 1,
        .sectors_processed = 7};
    EXPECT_EQ(resultA.isEmpty(), false);

    const TerminationResult resultB{.sectors = {{1, {12}}, {0, {10}}},
                                    .partitions_processed = 1,
                                    .sectors_processed = 2};
    EXPECT_EQ(resultB.isEmpty(), false);

    const TerminationResult expected{
        .sectors = {{2, {3, 5, 7}}, {0, {1, 2, 4, 10}}, {1, {12}}, {3, {9}}},
        .partitions_processed = 2,
        .sectors_processed = 9};

    result.add(resultA);
    result.add(resultB);

    EXPECT_EQ(result.partitions_processed, expected.partitions_processed);
    EXPECT_EQ(result.sectors_processed, expected.sectors_processed);
    EXPECT_EQ(result.sectors, expected.sectors);

    // partitions = 2, sectors = 9
    EXPECT_EQ(result.belowLimit(2, 9), false);
    EXPECT_EQ(result.belowLimit(3, 9), false);
    EXPECT_EQ(result.belowLimit(3, 8), false);
    EXPECT_EQ(result.belowLimit(2, 10), false);
    EXPECT_EQ(result.belowLimit(3, 10), true);
  }

}  // namespace fc::vm::actor::builtin::types::miner
