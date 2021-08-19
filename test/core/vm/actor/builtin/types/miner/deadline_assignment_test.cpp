/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline_assignment_heap.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

namespace fc::vm::actor::builtin::types::miner {

  struct DeadlineAssignmentTest : testing::Test {
    struct TestDeadline {
      uint64_t live_sectors{};
      uint64_t dead_sectors{};
      RleBitset expect_sectors;

      bool isEmpty() const {
        return live_sectors == 0 && dead_sectors == 0 && expect_sectors.empty();
      }
    };

    struct TestCase {
      TestCase() {
        deadlines =
            std::vector<TestDeadline>(kWPoStPeriodDeadlines, TestDeadline{});
      }

      uint64_t sectors{};
      std::vector<TestDeadline> deadlines;
    };

    void setupDeadlines(ActorVersion version,
                        uint64_t live_sectors,
                        uint64_t total_sectors) {
      Universal<Deadline> empty_dline{version};
      empty_dline->live_sectors = live_sectors;
      empty_dline->total_sectors = total_sectors;
      deadlines =
          std::vector<Universal<Deadline>>(kWPoStPeriodDeadlines, empty_dline);
    }

    std::vector<TestCase> initTestCases() const {
      std::vector<TestCase> test_cases;

      {
        TestCase test_case;
        test_case.sectors = 10;
        test_case.deadlines[0] =
            TestDeadline{.live_sectors = 0,
                         .dead_sectors = 0,
                         .expect_sectors = {0, 1, 2, 3, 8, 9}};
        test_case.deadlines[1] = TestDeadline{.live_sectors = 0,
                                              .dead_sectors = 0,
                                              .expect_sectors = {4, 5, 6, 7}};
        test_cases.push_back(test_case);
      }

      {
        TestCase test_case;
        test_case.sectors = 5;
        test_case.deadlines[0] = TestDeadline{
            .live_sectors = 0, .dead_sectors = 0, .expect_sectors = {3, 4}};
        test_case.deadlines[3] = TestDeadline{
            .live_sectors = 1, .dead_sectors = 0, .expect_sectors = {0, 1, 2}};
        test_cases.push_back(test_case);
      }

      {
        TestCase test_case;
        test_case.sectors = 1;
        test_case.deadlines[0] = TestDeadline{
            .live_sectors = 8, .dead_sectors = 0, .expect_sectors = {}};
        test_case.deadlines[1] = TestDeadline{
            .live_sectors = 7, .dead_sectors = 5, .expect_sectors = {0}};
        test_cases.push_back(test_case);
      }

      {
        TestCase test_case;
        test_case.sectors = 1;
        test_case.deadlines[0] = TestDeadline{
            .live_sectors = 4, .dead_sectors = 4, .expect_sectors = {}};
        test_case.deadlines[1] = TestDeadline{
            .live_sectors = 4, .dead_sectors = 0, .expect_sectors = {0}};
        test_cases.push_back(test_case);
      }

      {
        TestCase test_case;
        test_case.sectors = 1;
        test_case.deadlines[0] = TestDeadline{
            .live_sectors = 1, .dead_sectors = 0, .expect_sectors = {}};
        test_case.deadlines[1] = TestDeadline{
            .live_sectors = 2, .dead_sectors = 0, .expect_sectors = {0}};
        test_cases.push_back(test_case);
      }

      {
        TestCase test_case;
        test_case.sectors = 1;
        test_case.deadlines[0] = TestDeadline{
            .live_sectors = 1, .dead_sectors = 0, .expect_sectors = {}};
        test_case.deadlines[1] = TestDeadline{
            .live_sectors = 0, .dead_sectors = 2, .expect_sectors = {0}};
        test_cases.push_back(test_case);
      }

      {
        TestCase test_case;
        test_case.sectors = 1;
        test_case.deadlines[0] = TestDeadline{
            .live_sectors = 0, .dead_sectors = 1, .expect_sectors = {}};
        test_case.deadlines[1] = TestDeadline{
            .live_sectors = 0, .dead_sectors = 2, .expect_sectors = {0}};
        test_cases.push_back(test_case);
      }

      {
        TestCase test_case;
        test_case.sectors = 1;
        test_case.deadlines[0] = TestDeadline{
            .live_sectors = 1, .dead_sectors = 1, .expect_sectors = {}};
        test_case.deadlines[1] = TestDeadline{
            .live_sectors = 0, .dead_sectors = 2, .expect_sectors = {0}};
        test_cases.push_back(test_case);
      }

      return test_cases;
    }

    uint64_t max_partitions = 5;
    uint64_t partition_size = 5;
    std::vector<Universal<Deadline>> deadlines;
    std::vector<ActorVersion> versions{ActorVersion::kVersion0,
                                       ActorVersion::kVersion2,
                                       ActorVersion::kVersion3,
                                       ActorVersion::kVersion4,
                                       ActorVersion::kVersion5};
  };

  TEST_F(DeadlineAssignmentTest, TestDeadlineAssignment) {
    max_partitions = 4;
    partition_size = 100;

    const auto test_cases = initTestCases();

    for (const auto &version : versions) {
      for (const auto &test_case : test_cases) {
        setupDeadlines(version, 0, 0);
        for (size_t i = 0; i < deadlines.size(); i++) {
          const auto &test_dline = test_case.deadlines[i];
          if (test_dline.isEmpty()) {
            continue;
          }

          Universal<Deadline> deadline{version};
          deadline->live_sectors = test_dline.live_sectors;
          deadline->total_sectors =
              test_dline.live_sectors + test_dline.dead_sectors;

          deadlines[i] = deadline;
        }

        std::vector<SectorOnChainInfo> sectors_to_assign;
        for (uint64_t i = 0; i < test_case.sectors; i++) {
          SectorOnChainInfo sector;
          sector.sector = i;
          sectors_to_assign.push_back(sector);
        }

        EXPECT_OUTCOME_TRUE(
            assignment,
            assignDeadlines(
                max_partitions, partition_size, deadlines, sectors_to_assign));
        for (size_t i = 0; i < assignment.size(); i++) {
          const auto &sectors = assignment[i];
          const auto &test_dl = test_case.deadlines[i];
          if (test_dl.isEmpty()) {
            EXPECT_TRUE(sectors.empty());
            continue;
          }
          EXPECT_EQ(sectors.size(), test_dl.expect_sectors.size());

          RleBitset sector_nos;
          for (const auto &sector : sectors) {
            sector_nos.insert(sector.sector);
          }

          EXPECT_EQ(sector_nos, test_dl.expect_sectors);
        }
      }
    }
  }

  TEST_F(
      DeadlineAssignmentTest,
      FailsIfAllDeadlinesHitTheirMaxPartitionsLimitBeforeAssigningAllSectorsToDeadlines) {
    // one deadline can take 5 * 5 = 25 sectors
    // so 48 deadlines can take 48 * 25 = 1200 sectors.
    // Hence, we should fail if we try to assign 1201 sectors.

    std::vector<SectorOnChainInfo> sectors;
    for (size_t i = 0; i < 1201; i++) {
      SectorOnChainInfo sector;
      sector.sector = i;
      sectors.push_back(sector);
    }

    for (const auto &version : versions) {
      setupDeadlines(version, 0, 0);
      const auto result =
          assignDeadlines(max_partitions, partition_size, deadlines, sectors);
      EXPECT_EQ(result.error().message(),
                "max partitions limit reached for all deadlines");
    }
  }

  TEST_F(
      DeadlineAssignmentTest,
      SucceedsIfAllDeadlinesHitTheirMaxPartitionsLimitButAssignmentIsComplete) {
    // one deadline can take 5 * 5 = 25 sectors
    // so 48 deadlines that can take 48 * 25 = 1200 sectors.

    std::vector<SectorOnChainInfo> sectors;
    for (size_t i = 0; i < 1200; i++) {
      SectorOnChainInfo sector;
      sector.sector = i;
      sectors.push_back(sector);
    }

    for (const auto &version : versions) {
      setupDeadlines(version, 0, 0);
      EXPECT_OUTCOME_TRUE(
          deadline_to_sectors,
          assignDeadlines(max_partitions, partition_size, deadlines, sectors));
      for (const auto &sectors : deadline_to_sectors) {
        EXPECT_EQ(sectors.size(),
                  25);  // there should be (1200/48) = 25 sectors per deadline
      }
    }
  }

  TEST_F(
      DeadlineAssignmentTest,
      FailsIfSomeDeadlinesHaveSectorsBeforehAndAllDeadlinesHitTheirMaxPartitionLimit) {
    // can only take 1200 - (2 * 48) = 1104 sectors

    std::vector<SectorOnChainInfo> sectors;
    for (size_t i = 0; i < 1105; i++) {
      SectorOnChainInfo sector;
      sector.sector = i;
      sectors.push_back(sector);
    }

    for (const auto &version : versions) {
      setupDeadlines(version, 1, 2);
      const auto result =
          assignDeadlines(max_partitions, partition_size, deadlines, sectors);
      EXPECT_EQ(result.error().message(),
                "max partitions limit reached for all deadlines");
    }
  }

}  // namespace fc::vm::actor::builtin::types::miner
