/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/expiration.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

namespace fc::vm::actor::builtin::types::miner {

  struct ExpirationSetTest : testing::Test {
    RleBitset on_time_sectors{5, 8, 9};
    RleBitset early_sectors{2, 3};
    TokenAmount on_time_pledge{1000};
    PowerPair active_power = PowerPair(1 << 13, 1 << 14);
    PowerPair faulty_power = PowerPair(1 << 11, 1 << 12);

    ExpirationSet default_set{.on_time_sectors = on_time_sectors,
                              .early_sectors = early_sectors,
                              .on_time_pledge = on_time_pledge,
                              .active_power = active_power,
                              .faulty_power = faulty_power};
  };

  TEST_F(ExpirationSetTest, AddSectorsToEmpty) {
    ExpirationSet es;

    EXPECT_OUTCOME_TRUE_1(es.add(on_time_sectors,
                                 early_sectors,
                                 on_time_pledge,
                                 active_power,
                                 faulty_power));

    EXPECT_EQ(es.on_time_sectors, on_time_sectors);
    EXPECT_EQ(es.early_sectors, early_sectors);
    EXPECT_EQ(es.on_time_pledge, on_time_pledge);
    EXPECT_EQ(es.active_power, active_power);
    EXPECT_EQ(es.faulty_power, faulty_power);
    EXPECT_EQ(es.count(), 5);
  }

  TEST_F(ExpirationSetTest, AddSectorsToNonEmpty) {
    ExpirationSet es = default_set;

    EXPECT_OUTCOME_TRUE_1(es.add({6, 7, 11},
                                 {1, 4},
                                 300,
                                 PowerPair(3 * (1 << 13), 3 * (1 << 14)),
                                 PowerPair(3 * (1 << 11), 3 * (1 << 12))));

    const RleBitset expected_on_time_sectors{5, 6, 7, 8, 9, 11};
    const RleBitset expected_early_sectors{1, 2, 3, 4};

    EXPECT_EQ(es.on_time_sectors, expected_on_time_sectors);
    EXPECT_EQ(es.early_sectors, expected_early_sectors);
    EXPECT_EQ(es.on_time_pledge, 1300);
    EXPECT_EQ(es.active_power, PowerPair(1 << 15, 1 << 16));
    EXPECT_EQ(es.faulty_power, PowerPair(1 << 13, 1 << 14));
  }

  TEST_F(ExpirationSetTest, RemoveSectors) {
    ExpirationSet es = default_set;

    EXPECT_OUTCOME_TRUE_1(es.remove({9},
                                    {2},
                                    800,
                                    PowerPair(3 * (1 << 11), 3 * (1 << 12)),
                                    PowerPair(3 * (1 << 9), 3 * (1 << 10))));

    const RleBitset expected_on_time_sectors{5, 8};
    const RleBitset expected_early_sectors{3};

    EXPECT_EQ(es.on_time_sectors, expected_on_time_sectors);
    EXPECT_EQ(es.early_sectors, expected_early_sectors);
    EXPECT_EQ(es.on_time_pledge, 200);
    EXPECT_EQ(es.active_power, PowerPair(1 << 11, 1 << 12));
    EXPECT_EQ(es.faulty_power, PowerPair(1 << 9, 1 << 10));
  }

  TEST_F(ExpirationSetTest, RemoveFailsPledgeUnderflows) {
    ExpirationSet es = default_set;

    const auto res = es.remove({9},
                               {2},
                               1200,
                               PowerPair(3 * (1 << 11), 3 * (1 << 12)),
                               PowerPair(3 * (1 << 9), 3 * (1 << 10)));

    EXPECT_EQ(res.error().message(), "expiration set pledge underflow");
  }

  TEST_F(ExpirationSetTest, RemoveFailsToRemoveSectors) {
    ExpirationSet es = default_set;

    // remove unknown active sector 12
    const auto res1 = es.remove({12},
                                {},
                                0,
                                PowerPair(3 * (1 << 11), 3 * (1 << 12)),
                                PowerPair(3 * (1 << 9), 3 * (1 << 10)));

    EXPECT_EQ(res1.error().message(),
              "removing on-time sectors that are not contained");

    // remove faulty sector 8, that is active in the set
    const auto res2 = es.remove({},
                                {8},
                                0,
                                PowerPair(3 * (1 << 11), 3 * (1 << 12)),
                                PowerPair(3 * (1 << 9), 3 * (1 << 10)));

    EXPECT_EQ(res2.error().message(),
              "removing early sectors that are not contained");
  }

  TEST_F(ExpirationSetTest, RemoveFailsPowerUnderflows) {
    ExpirationSet es = default_set;

    // active removed power > active power
    const auto res1 = es.remove({9},
                                {2},
                                200,
                                PowerPair(3 * (1 << 12), 3 * (1 << 13)),
                                PowerPair(3 * (1 << 9), 3 * (1 << 10)));

    EXPECT_EQ(res1.error().message(), "expiration set power underflow");

    es = default_set;

    // faulty removed power > faulty power
    const auto res2 = es.remove({9},
                                {2},
                                200,
                                PowerPair(3 * (1 << 11), 3 * (1 << 12)),
                                PowerPair(3 * (1 << 10), 3 * (1 << 11)));

    EXPECT_EQ(res2.error().message(), "expiration set power underflow");
  }

  TEST_F(ExpirationSetTest, EmptySet) {
    ExpirationSet es;

    EXPECT_TRUE(es.isEmpty());
    EXPECT_EQ(es.count(), 0);

    EXPECT_OUTCOME_TRUE_1(es.add(on_time_sectors,
                                 early_sectors,
                                 on_time_pledge,
                                 active_power,
                                 faulty_power));

    EXPECT_FALSE(es.isEmpty());

    EXPECT_OUTCOME_TRUE_1(es.remove(on_time_sectors,
                                    early_sectors,
                                    on_time_pledge,
                                    active_power,
                                    faulty_power));

    EXPECT_TRUE(es.isEmpty());
    EXPECT_EQ(es.count(), 0);
  }

}  // namespace fc::vm::actor::builtin::types::miner
