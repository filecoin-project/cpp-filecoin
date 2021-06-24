/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline_sector_map.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

namespace fc::vm::actor::builtin::types::miner {

  struct DeadlineSectorMapTest : testing::Test {
    DeadlineSectorMap dsm;
  };

  // Test check() method
  TEST_F(DeadlineSectorMapTest, Check) {
    uint64_t dl_count{10};
    uint64_t part_count{5};
    for (uint64_t dl_id = 0; dl_id < dl_count; dl_id++) {
      for (uint64_t part_id = 0; part_id < part_count; part_id++) {
        EXPECT_OUTCOME_TRUE_1(
            dsm.add(dl_id, part_id, {dl_id * part_count + part_id}));
      }
    }

    for (const auto &[dl_id, partitions] : dsm.map) {
      for (const auto &[part_id, sector_nos] : partitions.map) {
        const RleBitset expected_sector_nos{dl_id * part_count + part_id};
        EXPECT_EQ(sector_nos, expected_sector_nos);
      }
    }

    EXPECT_OUTCOME_TRUE(result, dsm.count());
    const auto &[parts, sectors] = result;
    EXPECT_EQ(parts, part_count * dl_count);
    EXPECT_EQ(sectors, part_count * dl_count);

    auto res = dsm.check(1, 1);
    EXPECT_EQ(res.error().message(), "too many partitions");
    res = dsm.check(100, 1);
    EXPECT_EQ(res.error().message(), "too many sectors");
    res = dsm.check(1, 100);
    EXPECT_EQ(res.error().message(), "too many partitions");
    EXPECT_OUTCOME_TRUE_1(
        dsm.check(part_count * dl_count, part_count * dl_count));

    EXPECT_OUTCOME_TRUE_1(dsm.add(0, 0, {1000}));
    const RleBitset expected_sector_nos{0, 1000};
    EXPECT_EQ(dsm.map[0].map[0], expected_sector_nos);
    res = dsm.check(part_count * dl_count, part_count * dl_count);
    EXPECT_EQ(res.error().message(), "too many sectors");
    EXPECT_OUTCOME_TRUE_1(
        dsm.check(part_count * dl_count, part_count * dl_count + 1));
  }

  // Test add() method
  TEST_F(DeadlineSectorMapTest, Add) {
    const RleBitset sector_nos{0, 1, 2, 3};

    EXPECT_OUTCOME_TRUE_1(dsm.add(0, 1, sector_nos));

    const auto res = dsm.add(48, 1, sector_nos);
    EXPECT_EQ(res.error().message(), "invalid deadline");

    EXPECT_EQ(dsm.map[0].map[1], sector_nos);
  }

  // Test count() method
  TEST_F(DeadlineSectorMapTest, Count) {
    RleBitset sector_nos;
    for (uint64_t i = 0; i < 100; i++) {
      sector_nos.insert(i);
    }

    EXPECT_OUTCOME_TRUE_1(dsm.add(0, 1, sector_nos));
    EXPECT_OUTCOME_TRUE_1(dsm.add(1, 1, sector_nos));

    EXPECT_OUTCOME_TRUE(result, dsm.count());
    const auto &[partitions, sectors] = result;

    EXPECT_EQ(partitions, 2);
    EXPECT_EQ(sectors, 200);
  }

  // Test empty map
  TEST_F(DeadlineSectorMapTest, Empty) {
    EXPECT_OUTCOME_TRUE(result, dsm.count());
    const auto &[partitions, sectors] = result;

    EXPECT_EQ(partitions, 0);
    EXPECT_EQ(sectors, 0);
    EXPECT_TRUE(dsm.deadlines().empty());
  }

  // Test deadlines() method
  TEST_F(DeadlineSectorMapTest, Deadlines) {
    std::vector<uint64_t> expected_deadlines;

    for (uint64_t i = 47; i > 0; i--) {
      EXPECT_OUTCOME_TRUE_1(dsm.add(i, 0, {0}));
    }

    for (uint64_t i = 1; i <= 47; i++) {
      expected_deadlines.push_back(i);
    }

    const auto deadlines = dsm.deadlines();

    EXPECT_EQ(deadlines, expected_deadlines);
  }

}  // namespace fc::vm::actor::builtin::types::miner
