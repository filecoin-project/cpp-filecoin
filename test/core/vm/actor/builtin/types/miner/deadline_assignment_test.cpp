/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline_assignment.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::types::miner {

  auto sectors(const std::vector<SectorNumber> &numbers) {
    std::vector<SectorOnChainInfo> sectors;
    for (const auto &number : numbers) {
      SectorOnChainInfo sector;
      sector.sector = number;
      sector.seal_proof = RegisteredSealProof::kUndefined;
      sector.sealed_cid = kEmptyObjectCid;
      sectors.push_back(sector);
    }
    return sectors;
  }

  auto sectors(size_t count) {
    std::vector<SectorOnChainInfo> sectors;
    for (SectorNumber i = 0; i < count; i++) {
      SectorOnChainInfo sector;
      sector.sector = i;
      sector.seal_proof = RegisteredSealProof::kUndefined;
      sector.sealed_cid = kEmptyObjectCid;
      sectors.push_back(sector);
    }
    return sectors;
  }

  struct TestCase {
    std::map<uint64_t, Universal<Deadline>> deadlines;
    std::vector<std::vector<SectorOnChainInfo>> expected;

    auto sectorsCount() const {
      size_t sum{0};
      for (const auto &dl : expected) {
        sum += dl.size();
      }
      return sum;
    }

    const std::vector<SectorOnChainInfo> *expect(size_t i) const {
      size_t j = 0;
      for (const auto &[dl_id, dl] : deadlines) {
        if (dl_id == i) {
          return &expected[j];
        }
        j++;
      }
      return nullptr;
    }
  };

  struct DeadlineAssignmentTestP : ::testing::TestWithParam<TestCase> {
    static std::vector<TestCase> initTestCases() {
      std::vector<TestCase> test_cases;

      auto dl{[](size_t live, size_t dead) {
        Universal<Deadline> deadline{ActorVersion::kVersion0};
        deadline->live_sectors = live;
        deadline->total_sectors = live + dead;
        return deadline;
      }};

      test_cases.push_back(TestCase{
          {{0, dl(0, 0)}, {1, dl(0, 0)}},
          {sectors({0, 1, 2, 3, 8, 9}), sectors({4, 5, 6, 7})},
      });

      test_cases.push_back({
          {{0, dl(0, 0)}, {3, dl(1, 0)}},
          {sectors({3, 4}), sectors({0, 1, 2})},
      });

      test_cases.push_back({
          {{0, dl(8, 0)}, {1, dl(7, 5)}},
          {{}, sectors(std::vector<SectorNumber>{0})},
      });

      test_cases.push_back({
          {{0, dl(4, 4)}, {1, dl(4, 0)}},
          {{}, sectors(std::vector<SectorNumber>{0})},
      });

      test_cases.push_back({
          {{0, dl(1, 0)}, {1, dl(2, 0)}},
          {{}, sectors(std::vector<SectorNumber>{0})},
      });

      test_cases.push_back({
          {{0, dl(1, 0)}, {1, dl(0, 2)}},
          {{}, sectors(std::vector<SectorNumber>{0})},
      });

      test_cases.push_back({
          {{0, dl(0, 1)}, {1, dl(0, 2)}},
          {{}, sectors(std::vector<SectorNumber>{0})},
      });

      test_cases.push_back({
          {{0, dl(1, 1)}, {1, dl(0, 2)}},
          {{}, sectors(std::vector<SectorNumber>{0})},
      });

      return test_cases;
    }
  };

  TEST_P(DeadlineAssignmentTestP, TestDeadlineAssignment) {
    const auto &test_case{GetParam()};
    EXPECT_OUTCOME_TRUE(
        assignment,
        assignDeadlines(
            100, 4, test_case.deadlines, sectors(test_case.sectorsCount())));
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
      std::map<uint64_t, Universal<Deadline>> deadlines;

      auto dl{[](size_t live, size_t total) {
        Universal<Deadline> deadline{ActorVersion::kVersion0};
        deadline->live_sectors = live;
        deadline->total_sectors = total;
        return deadline;
      }};

      for (size_t i = 0; i < kWPoStPeriodDeadlines; ++i) {
        deadlines[i] = dl(live, total);
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
        max_partitions, partition_size, fillDeadlines(0, 0), sectors(1201));
    EXPECT_EQ(result.error().message(),
              "max partitions limit reached for all deadlines");
  }

  TEST_F(
      DeadlineAssignmentTest,
      SucceedsIfAllDeadlinesHitTheirMaxPartitionsLimitButAssignmentIsComplete) {
    // one deadline can take 5 * 5 = 25 sectors
    // so 48 deadlines that can take 48 * 25 = 1200 sectors.

    EXPECT_OUTCOME_TRUE(deadline_to_sectors,
                        assignDeadlines(max_partitions,
                                        partition_size,
                                        fillDeadlines(0, 0),
                                        sectors(1200)));
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
        max_partitions, partition_size, fillDeadlines(1, 2), sectors(1105));
    EXPECT_EQ(result.error().message(),
              "max partitions limit reached for all deadlines");
  }

}  // namespace fc::vm::actor::builtin::types::miner
