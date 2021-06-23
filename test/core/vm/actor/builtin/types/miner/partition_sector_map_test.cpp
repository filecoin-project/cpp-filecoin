/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/partition_sector_map.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

namespace fc::vm::actor::builtin::types::miner {

  struct PartitionSectorMapTest : testing::Test {
    PartitionSectorMap psm;
  };

  TEST_F(PartitionSectorMapTest, Add) {
    const RleBitset sector_nos{0, 1, 2, 3};

    psm.add(0, sector_nos);

    EXPECT_EQ(psm.map[0], sector_nos);
  }

  TEST_F(PartitionSectorMapTest, Count) {
    RleBitset sector_nos;
    for (uint64_t i = 0; i < 100; i++) {
      sector_nos.insert(i);
    }

    psm.add(0, sector_nos);
    psm.add(1, sector_nos);

    EXPECT_OUTCOME_TRUE(result, psm.count());
    const auto &[partitions, sectors] = result;

    EXPECT_EQ(partitions, 2);
    EXPECT_EQ(sectors, 200);
  }

  TEST_F(PartitionSectorMapTest, Empty) {
    EXPECT_OUTCOME_TRUE(result, psm.count());
    const auto &[partitions, sectors] = result;

    EXPECT_EQ(partitions, 0);
    EXPECT_EQ(sectors, 0);
    EXPECT_TRUE(psm.partitions().empty());
  }

  TEST_F(PartitionSectorMapTest, Partitions) {
    std::vector<uint64_t> expected_partitions;

    for (uint64_t i = 100; i > 0; i--) {
      psm.add(i, {0});
    }

    for (uint64_t i = 1; i <= 100; i++) {
      expected_partitions.push_back(i);
    }

    const auto partitions = psm.partitions();

    EXPECT_EQ(partitions, expected_partitions);
  }

}  // namespace fc::vm::actor::builtin::types::miner
