/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline.hpp"

#include <gtest/gtest.h>

#include "expected_deadline_v0.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "test_utils.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::RleBitset;
  using primitives::SectorSize;
  using runtime::MockRuntime;
  using storage::ipfs::InMemoryDatastore;
  using types::Universal;
  using types::miner::Deadline;
  using types::miner::PartitionSectorMap;
  using types::miner::PoStPartition;
  using types::miner::powerForSectors;
  using types::miner::PowerPair;
  using types::miner::QuantSpec;
  using types::miner::SectorOnChainInfo;
  using types::miner::Sectors;
  using types::miner::TerminationResult;

  struct DeadlineTestV0 : testing::Test {
    void SetUp() override {
      actor_version = ActorVersion::kVersion0;
      ipld->actor_version = actor_version;
      deadline = Universal<Deadline>(actor_version);
      cbor_blake::cbLoadT(ipld, deadline);

      EXPECT_CALL(runtime, getIpfsDatastore())
          .WillRepeatedly(testing::Invoke([&]() { return ipld; }));

      EXPECT_CALL(runtime, getActorVersion())
          .WillRepeatedly(testing::Invoke([&]() { return actor_version; }));

      sectors = {testSector(actor_version, 2, 1, 50, 60, 1000),
                 testSector(actor_version, 3, 2, 51, 61, 1001),
                 testSector(actor_version, 7, 3, 52, 62, 1002),
                 testSector(actor_version, 8, 4, 53, 63, 1003),
                 testSector(actor_version, 8, 5, 54, 64, 1004),
                 testSector(actor_version, 11, 6, 55, 65, 1005),
                 testSector(actor_version, 13, 7, 56, 66, 1006),
                 testSector(actor_version, 8, 8, 57, 67, 1007),
                 testSector(actor_version, 8, 9, 58, 68, 1008)};
    }

    void initExpectedDeadline() {
      expected_deadline = {};
      expected_deadline.quant = quant;
      expected_deadline.partition_size = partition_size;
      expected_deadline.ssize = ssize;
      expected_deadline.sectors = sectors;
    }

    Sectors sectorsArr() const {
      Sectors sectors_arr;
      cbor_blake::cbLoadT(ipld, sectors_arr);
      EXPECT_OUTCOME_TRUE_1(sectors_arr.store(sectors));
      return sectors_arr;
    }

    PowerPair sectorPower(const RleBitset &sector_nos) const {
      return powerForSectors(ssize, selectSectorsTest(sectors, sector_nos));
    }

    void addSectors() {
      EXPECT_OUTCOME_TRUE(
          power,
          deadline->addSectors(
              runtime, partition_size, false, sectors, ssize, quant));
      EXPECT_EQ(power, powerForSectors(ssize, sectors));

      initExpectedDeadline();
      expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
      expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
      expected_deadline.partition_sectors.push_back({9});
      expected_deadline.assertDeadline(deadline);
    }

    void addThenTerminate() {
      addSectors();

      PartitionSectorMap sector_map;
      sector_map.map[0] = {1, 3};
      sector_map.map[1] = {6};

      EXPECT_OUTCOME_TRUE(removed_power,
                          deadline->terminateSectors(
                              sectorsArr(), 15, sector_map, ssize, quant));
      EXPECT_EQ(removed_power, sectorPower({1, 3, 6}));

      initExpectedDeadline();
      expected_deadline.terminations = {1, 3, 6};
      expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
      expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
      expected_deadline.partition_sectors.push_back({9});
      expected_deadline.assertDeadline(deadline);
    }

    void addThenTerminateThenPopEarly() {
      addThenTerminate();

      EXPECT_OUTCOME_TRUE(result, deadline->popEarlyTerminations(100, 100));
      auto &[early_terminations, more] = result;
      EXPECT_FALSE(more);
      EXPECT_EQ(early_terminations.partitions_processed, 2);
      EXPECT_EQ(early_terminations.sectors_processed, 3);
      EXPECT_EQ(early_terminations.sectors.size(), 1);
      const RleBitset expected_terminated_sectors{1, 3, 6};
      EXPECT_EQ(early_terminations.sectors[15], expected_terminated_sectors);

      initExpectedDeadline();
      expected_deadline.terminations = {1, 3, 6};
      expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
      expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
      expected_deadline.partition_sectors.push_back({9});
      expected_deadline.assertDeadline(deadline);
    }

    void addThenTerminateThenRemovePartition() {
      addThenTerminateThenPopEarly();

      EXPECT_OUTCOME_TRUE(result,
                          deadline->removePartitions(runtime, {0}, quant));
      const auto &[live, dead, removed_power] = result;

      const RleBitset expected_live{2, 4};
      EXPECT_EQ(live, expected_live);

      const RleBitset expected_dead{1, 3};
      EXPECT_EQ(dead, expected_dead);

      const auto live_power =
          powerForSectors(ssize, selectSectorsTest(sectors, live));
      EXPECT_EQ(removed_power, live_power);

      initExpectedDeadline();
      expected_deadline.terminations = {6};
      expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
      expected_deadline.partition_sectors.push_back({9});
      expected_deadline.assertDeadline(deadline);
    }

    void addThenMarkFaulty() {
      addSectors();

      PartitionSectorMap sector_map;
      sector_map.map[0] = {1};
      sector_map.map[1] = {5, 6};

      EXPECT_OUTCOME_TRUE(
          faulty_power,
          deadline->recordFaults(sectorsArr(), ssize, quant, 9, sector_map));
      EXPECT_EQ(faulty_power, sectorPower({1, 5, 6}));

      initExpectedDeadline();
      expected_deadline.faults = {1, 5, 6};
      expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
      expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
      expected_deadline.partition_sectors.push_back({9});
      expected_deadline.assertDeadline(deadline);
    }

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    ActorVersion actor_version;

    std::vector<Universal<SectorOnChainInfo>> sectors;
    SectorSize ssize{static_cast<uint64_t>(32) << 30};
    QuantSpec quant{4, 1};
    uint64_t partition_size{4};

    Universal<Deadline> deadline;
    ExpectedDeadline expected_deadline;
  };

  TEST_F(DeadlineTestV0, AddsSectors) {
    addSectors();
  }

  TEST_F(DeadlineTestV0, TerminatesSectors) {
    addThenTerminate();
  }

  TEST_F(DeadlineTestV0, PopsEarlyTerminations) {
    addThenTerminateThenPopEarly();
  }

  TEST_F(DeadlineTestV0, RemovesPartitions) {
    addThenTerminateThenRemovePartition();
  }

  TEST_F(DeadlineTestV0, MarksFaulty) {
    addThenMarkFaulty();
  }

  TEST_F(DeadlineTestV0, CannotRemovePartitionsWithEarlyTerminations) {
    addThenTerminate();

    const auto result = deadline->removePartitions(runtime, {0}, quant);
    EXPECT_EQ(result.error().message(),
              "cannot remove partitions from deadline with early terminations");
  }

  TEST_F(DeadlineTestV0, CanPopEarlyTerminationsInMultipleSteps) {
    addThenTerminate();

    TerminationResult result;

    EXPECT_OUTCOME_TRUE(pop_result1, deadline->popEarlyTerminations(2, 1));
    const auto &[result1, has_more1] = pop_result1;
    EXPECT_TRUE(has_more1);
    result.add(result1);

    EXPECT_OUTCOME_TRUE(pop_result2, deadline->popEarlyTerminations(2, 1));
    const auto &[result2, has_more2] = pop_result2;
    EXPECT_TRUE(has_more2);
    result.add(result2);

    EXPECT_OUTCOME_TRUE(pop_result3, deadline->popEarlyTerminations(1, 1));
    const auto &[result3, has_more3] = pop_result3;
    EXPECT_FALSE(has_more3);
    result.add(result3);

    EXPECT_EQ(result.partitions_processed, 3);
    EXPECT_EQ(result.sectors_processed, 3);
    EXPECT_EQ(result.sectors.size(), 1);
    const RleBitset expected_sectors{1, 3, 6};
    EXPECT_EQ(result.sectors[15], expected_sectors);

    initExpectedDeadline();
    expected_deadline.terminations = {1, 3, 6};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);
  }

  TEST_F(DeadlineTestV0, CannotRemoveMissingPartition) {
    addThenTerminateThenRemovePartition();

    const auto result = deadline->removePartitions(runtime, {2}, quant);
    EXPECT_EQ(result.error().message(), "partition index is out of range");
  }

  TEST_F(DeadlineTestV0, RemovingNoPartitionsDoesNothing) {
    addThenTerminateThenPopEarly();

    EXPECT_OUTCOME_TRUE(result, deadline->removePartitions(runtime, {}, quant));
    const auto &[live, dead, removed_power] = result;

    EXPECT_TRUE(removed_power.isZero());
    EXPECT_TRUE(live.empty());
    EXPECT_TRUE(dead.empty());

    initExpectedDeadline();
    expected_deadline.terminations = {1, 3, 6};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);
  }

  TEST_F(DeadlineTestV0, FailsToRemovePartitionsWithFaultySectors) {
    addThenMarkFaulty();

    const auto result = deadline->removePartitions(runtime, {1}, quant);
    EXPECT_EQ(result.error().message(), "cannot remove, partition has faults");
  }

  TEST_F(DeadlineTestV0, TerminateFaulty) {
    addThenMarkFaulty();  // 1, 5, 6 faulty

    PartitionSectorMap sector_map;
    sector_map.map[0] = {1, 3};
    sector_map.map[1] = {6};

    EXPECT_OUTCOME_TRUE(
        removed_power,
        deadline->terminateSectors(sectorsArr(), 15, sector_map, ssize, quant));
    EXPECT_EQ(removed_power,
              powerForSectors(ssize, selectSectorsTest(sectors, {3})));

    initExpectedDeadline();
    expected_deadline.terminations = {1, 3, 6};
    expected_deadline.faults = {5};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);
  }

  TEST_F(DeadlineTestV0, FaultySectorsExpire) {
    addThenMarkFaulty();

    EXPECT_OUTCOME_TRUE(exp, deadline->popExpiredSectors(9, quant));

    const RleBitset expected_on_time{1, 2, 3, 4, 5, 8, 9};
    EXPECT_EQ(exp.on_time_sectors, expected_on_time);
    const RleBitset expected_early{6};
    EXPECT_EQ(exp.early_sectors, expected_early);

    initExpectedDeadline();
    expected_deadline.terminations = {1, 2, 3, 4, 5, 6, 8, 9};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);

    EXPECT_OUTCOME_TRUE(result, deadline->popEarlyTerminations(100, 100));
    auto &[early_terminations, more] = result;
    EXPECT_FALSE(more);
    EXPECT_EQ(early_terminations.partitions_processed, 1);
    EXPECT_EQ(early_terminations.sectors_processed, 1);
    EXPECT_EQ(early_terminations.sectors.size(), 1);
    const RleBitset expected_sectors{6};
    EXPECT_EQ(early_terminations.sectors[9], expected_sectors);

    initExpectedDeadline();
    expected_deadline.terminations = {1, 2, 3, 4, 5, 6, 8, 9};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);
  }

  TEST_F(DeadlineTestV0, PostAllTheThings) {
    addSectors();

    const std::vector<PoStPartition> post_partitions1{
        {.index = 0, .skipped = {}}, {.index = 1, .skipped = {}}};

    EXPECT_OUTCOME_TRUE(post_result1,
                        deadline->recordProvenSectors(
                            sectorsArr(), ssize, quant, 13, post_partitions1));
    const RleBitset expected_sectors1{1, 2, 3, 4, 5, 6, 7, 8};
    EXPECT_EQ(post_result1.sectors, expected_sectors1);
    EXPECT_TRUE(post_result1.ignored_sectors.empty());
    EXPECT_TRUE(post_result1.new_faulty_power.isZero());
    EXPECT_TRUE(post_result1.retracted_recovery_power.isZero());
    EXPECT_TRUE(post_result1.recovered_power.isZero());

    initExpectedDeadline();
    expected_deadline.posts = {0, 1};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);

    const std::vector<PoStPartition> post_partitions2{
        {.index = 1, .skipped = {}}, {.index = 2, .skipped = {}}};

    EXPECT_OUTCOME_TRUE(post_result2,
                        deadline->recordProvenSectors(
                            sectorsArr(), ssize, quant, 13, post_partitions2));
    const RleBitset expected_sectors2{9};
    EXPECT_EQ(post_result2.sectors, expected_sectors2);
    EXPECT_TRUE(post_result2.ignored_sectors.empty());
    EXPECT_TRUE(post_result2.new_faulty_power.isZero());
    EXPECT_TRUE(post_result2.retracted_recovery_power.isZero());
    EXPECT_TRUE(post_result2.recovered_power.isZero());

    initExpectedDeadline();
    expected_deadline.posts = {0, 1, 2};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);

    EXPECT_OUTCOME_TRUE(process_result,
                        deadline->processDeadlineEnd(quant, 13));
    const auto &[new_faulty_power, failed_recovery_power] = process_result;
    EXPECT_TRUE(new_faulty_power.isZero());
    EXPECT_TRUE(failed_recovery_power.isZero());

    initExpectedDeadline();
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);
  }

  TEST_F(DeadlineTestV0, PostWithFaultsRecoveriesAndRetractedRecoveries) {
    addThenMarkFaulty();

    PartitionSectorMap sector_map;
    sector_map.map[0] = {1};
    sector_map.map[1] = {6};

    EXPECT_OUTCOME_TRUE_1(
        deadline->declareFaultsRecovered(sectorsArr(), ssize, sector_map));

    initExpectedDeadline();
    expected_deadline.recovering = {1, 6};
    expected_deadline.faults = {1, 5, 6};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);

    const std::vector<PoStPartition> post_partitions{
        {.index = 0, .skipped = {1}}, {.index = 1, .skipped = {7}}};

    EXPECT_OUTCOME_TRUE(post_result,
                        deadline->recordProvenSectors(
                            sectorsArr(), ssize, quant, 13, post_partitions));
    const RleBitset expected_sectors{1, 2, 3, 4, 5, 6, 7, 8};
    EXPECT_EQ(post_result.sectors, expected_sectors);
    const RleBitset expected_ignored{1, 5, 7};
    EXPECT_EQ(post_result.ignored_sectors, expected_ignored);
    EXPECT_EQ(post_result.new_faulty_power, sectorPower({7}));
    EXPECT_EQ(post_result.retracted_recovery_power, sectorPower({1}));
    EXPECT_EQ(post_result.recovered_power, sectorPower({6}));

    initExpectedDeadline();
    expected_deadline.posts = {0, 1};
    expected_deadline.faults = {1, 5, 7};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);

    EXPECT_OUTCOME_TRUE(process_result,
                        deadline->processDeadlineEnd(quant, 13));
    const auto &[new_faulty_power, failed_recovery_power] = process_result;
    EXPECT_EQ(new_faulty_power, sectorPower({9}));
    EXPECT_TRUE(failed_recovery_power.isZero());

    initExpectedDeadline();
    expected_deadline.faults = {1, 5, 7, 9};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);
  }

  TEST_F(DeadlineTestV0, RetractRecoveries) {
    addThenMarkFaulty();

    PartitionSectorMap sector_map1;
    sector_map1.map[0] = {1};
    sector_map1.map[1] = {6};

    EXPECT_OUTCOME_TRUE_1(
        deadline->declareFaultsRecovered(sectorsArr(), ssize, sector_map1));

    PartitionSectorMap sector_map2;
    sector_map2.map[0] = {1};

    EXPECT_OUTCOME_TRUE(
        faulty_power,
        deadline->recordFaults(sectorsArr(), ssize, quant, 13, sector_map2));
    EXPECT_TRUE(faulty_power.isZero());

    initExpectedDeadline();
    expected_deadline.recovering = {6};
    expected_deadline.faults = {1, 5, 6};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);

    const std::vector<PoStPartition> post_partitions{
        {.index = 0, .skipped = {}},
        {.index = 1, .skipped = {}},
        {.index = 2, .skipped = {}}};

    EXPECT_OUTCOME_TRUE(post_result,
                        deadline->recordProvenSectors(
                            sectorsArr(), ssize, quant, 13, post_partitions));
    const RleBitset expected_sectors{1, 2, 3, 4, 5, 6, 7, 8, 9};
    EXPECT_EQ(post_result.sectors, expected_sectors);
    const RleBitset expected_ignored{1, 5};
    EXPECT_EQ(post_result.ignored_sectors, expected_ignored);
    EXPECT_TRUE(post_result.new_faulty_power.isZero());
    EXPECT_TRUE(post_result.retracted_recovery_power.isZero());
    EXPECT_EQ(post_result.recovered_power, sectorPower({6}));

    initExpectedDeadline();
    expected_deadline.posts = {0, 1, 2};
    expected_deadline.faults = {1, 5};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);

    EXPECT_OUTCOME_TRUE(process_result,
                        deadline->processDeadlineEnd(quant, 13));
    const auto &[new_faulty_power, failed_recovery_power] = process_result;
    EXPECT_TRUE(new_faulty_power.isZero());
    EXPECT_TRUE(failed_recovery_power.isZero());

    initExpectedDeadline();
    expected_deadline.faults = {1, 5};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);
  }

  TEST_F(DeadlineTestV0, RescheduleExpirations) {
    addThenMarkFaulty();

    PartitionSectorMap sector_map;
    sector_map.map[1] = {6, 7, 99};  // 99 should be skipped, it doesn't exist.
    sector_map.map[5] = {100};       // partition 5 doesn't exist.
    sector_map.map[2] = {};          // empty bitfield should be fine.

    EXPECT_OUTCOME_TRUE_1(deadline->rescheduleSectorExpirations(
        sectorsArr(), 1, sector_map, ssize, quant));

    EXPECT_OUTCOME_TRUE(exp, deadline->popExpiredSectors(1, quant));

    const auto sector7 = selectSectorsTest(sectors, {7})[0];

    initExpectedDeadline();
    expected_deadline.faults = {1, 5, 6};
    expected_deadline.terminations = {7};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(deadline);

    EXPECT_EQ(exp.active_power,
              PowerPair(ssize, qaPowerForSector(ssize, *sector7)));
    EXPECT_TRUE(exp.faulty_power.isZero());
    EXPECT_EQ(exp.on_time_pledge, sector7->init_pledge);
  }

}  // namespace fc::vm::actor::builtin::v0::miner
