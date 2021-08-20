/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline_assignment_heap.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

namespace fc::vm::actor::builtin::types::miner {
  struct TestCase {
    std::vector<DeadlineAssignmentInfo> deadlines;
    std::vector<std::vector<size_t>> expected;

    auto sectors() const {
      size_t sum{0};
      for (const auto &dl : expected) {
        sum += dl.size();
      }
      return sum;
    }

    const std::vector<size_t> *expect(size_t i) const {
      for (size_t j = 0; j < deadlines.size(); ++j) {
        const auto &dl{deadlines[j]};
        if (dl.index == i) {
          return &expected[j];
        }
      }
      return nullptr;
    }
  };

  struct DeadlineAssignmentTestP : ::testing::TestWithParam<TestCase> {
    static std::vector<TestCase> initTestCases() {
      std::vector<TestCase> test_cases;

      auto dl{[](size_t i, size_t live, size_t dead) {
        return DeadlineAssignmentInfo{i, live, live + dead};
      }};

      test_cases.push_back({
          {dl(0, 0, 0), dl(1, 0, 0)},
          {{0, 1, 2, 3, 8, 9}, {4, 5, 6, 7}},
      });

      test_cases.push_back({
          {dl(0, 0, 0), dl(3, 1, 0)},
          {{3, 4}, {0, 1, 2}},
      });

      test_cases.push_back({
          {dl(0, 8, 0), dl(1, 7, 5)},
          {{}, {0}},
      });

      test_cases.push_back({
          {dl(0, 4, 4), dl(1, 4, 0)},
          {{}, {0}},
      });

      test_cases.push_back({
          {dl(0, 1, 0), dl(1, 2, 0)},
          {{}, {0}},
      });

      test_cases.push_back({
          {dl(0, 1, 0), dl(1, 0, 2)},
          {{}, {0}},
      });

      test_cases.push_back({
          {dl(0, 0, 1), dl(1, 0, 2)},
          {{}, {0}},
      });

      test_cases.push_back({
          {dl(0, 1, 1), dl(1, 0, 2)},
          {{}, {0}},
      });

      return test_cases;
    }
  };

  TEST_P(DeadlineAssignmentTestP, TestDeadlineAssignment) {
    const auto &test_case{GetParam()};
    EXPECT_OUTCOME_TRUE(
        assignment,
        assignDeadlines(100, 4, test_case.deadlines, test_case.sectors()));
    EXPECT_EQ(assignment.size(), kWPoStPeriodDeadlines);
    for (size_t i = 0; i < assignment.size(); ++i) {
      if (const auto *expected{test_case.expect(i)}) {
        EXPECT_EQ(assignment[i], *expected);
      } else {
        EXPECT_TRUE(assignment[i].empty());
      }
    }
  }

  INSTANTIATE_TEST_CASE_P(
      P,
      DeadlineAssignmentTestP,
      ::testing::ValuesIn(DeadlineAssignmentTestP::initTestCases()));

  struct DeadlineAssignmentTest : ::testing::Test {
    auto fillDeadlines(uint64_t live, uint64_t total) {
      std::vector<DeadlineAssignmentInfo> deadlines;
      for (size_t i{0}; i < kWPoStPeriodDeadlines; ++i) {
        deadlines.push_back({i, live, total});
      }
      return deadlines;
    }

    uint64_t max_partitions = 5;
    uint64_t partition_size = 5;
  };

  TEST_F(
      DeadlineAssignmentTest,
      FailsIfAllDeadlinesHitTheirMaxPartitionsLimitBeforeAssigningAllSectorsToDeadlines) {
    // one deadline can take 5 * 5 = 25 sectors
    // so 48 deadlines can take 48 * 25 = 1200 sectors.
    // Hence, we should fail if we try to assign 1201 sectors.

    const auto result = assignDeadlines(
        max_partitions, partition_size, fillDeadlines(0, 0), 1201);
    EXPECT_EQ(result.error().message(),
              "max partitions limit reached for all deadlines");
  }

  TEST_F(
      DeadlineAssignmentTest,
      SucceedsIfAllDeadlinesHitTheirMaxPartitionsLimitButAssignmentIsComplete) {
    // one deadline can take 5 * 5 = 25 sectors
    // so 48 deadlines that can take 48 * 25 = 1200 sectors.

    EXPECT_OUTCOME_TRUE(
        deadline_to_sectors,
        assignDeadlines(
            max_partitions, partition_size, fillDeadlines(0, 0), 1200));
    for (const auto &sectors : deadline_to_sectors) {
      EXPECT_EQ(sectors.size(),
                25);  // there should be (1200/48) = 25 sectors per deadline
    }
  }

  TEST_F(
      DeadlineAssignmentTest,
      FailsIfSomeDeadlinesHaveSectorsBeforehAndAllDeadlinesHitTheirMaxPartitionLimit) {
    // can only take 1200 - (2 * 48) = 1104 sectors

    const auto result = assignDeadlines(
        max_partitions, partition_size, fillDeadlines(1, 2), 1105);
    EXPECT_EQ(result.error().message(),
              "max partitions limit reached for all deadlines");
  }

}  // namespace fc::vm::actor::builtin::types::miner
