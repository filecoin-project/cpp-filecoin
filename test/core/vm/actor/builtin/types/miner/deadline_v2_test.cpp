/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v2/deadline.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "test_utils.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/actor/builtin/types/type_manager/type_manager.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::RleBitset;
  using runtime::MockRuntime;
  using storage::ipfs::InMemoryDatastore;
  using types::TypeManager;
  using types::Universal;
  using types::miner::BitfieldQueue;
  using types::miner::ExpirationSet;
  using types::miner::kEearlyTerminatedBitWidth;
  using types::miner::kNoQuantization;
  using types::miner::Partition;
  using types::miner::PoStPartition;
  using types::miner::powerForSectors;
  using types::miner::TerminationResult;

  struct ExpectedDeadline {
    QuantSpec quant;
    SectorSize ssize{};
    uint64_t partition_size{};
    std::vector<SectorOnChainInfo> sectors;
    RleBitset faults;
    RleBitset recovering;
    RleBitset terminations;
    RleBitset unproven;
    RleBitset posts;
    std::vector<RleBitset> partition_sectors;

    void assertDeadline(MockRuntime &runtime, const Deadline &deadline) const {
      const auto [all_sectors,
                  all_faults,
                  all_recoveries,
                  all_terminations,
                  all_unproven,
                  partitions] = checkDeadlineInvariants(runtime, deadline);

      EXPECT_EQ(faults, all_faults);
      EXPECT_EQ(recovering, all_recoveries);
      EXPECT_EQ(terminations, all_terminations);
      EXPECT_EQ(unproven, all_unproven);
      EXPECT_EQ(posts, deadline.partitions_posted);
      EXPECT_EQ(partition_sectors.size(), partitions.size());

      for (size_t i = 0; i < partition_sectors.size(); i++) {
        EXPECT_EQ(partition_sectors[i], partitions[i]);
      }
    }

    std::tuple<RleBitset,
               RleBitset,
               RleBitset,
               RleBitset,
               RleBitset,
               std::vector<RleBitset>>
    checkDeadlineInvariants(MockRuntime &runtime,
                            const Deadline &deadline) const {
      std::map<ChainEpoch, std::vector<uint64_t>> expected_deadline_exp_queue;
      RleBitset partitions_with_early_terminations;

      RleBitset all_sectors;
      RleBitset all_faults;
      RleBitset all_recoveries;
      RleBitset all_terminations;
      RleBitset all_unproven;
      PowerPair all_faulty_power;
      std::vector<RleBitset> partition_sectors;

      int64_t expected_part_index = 0;

      auto partitions_visitor{
          [&](int64_t part_id, const auto &partition) -> outcome::result<void> {
            EXPECT_EQ(part_id, expected_part_index);
            expected_part_index++;

            partition_sectors.push_back(partition->sectors);

            EXPECT_FALSE(all_sectors.containsAny(partition->sectors));

            all_sectors += partition->sectors;
            all_faults += partition->faults;
            all_recoveries += partition->recoveries;
            all_terminations += partition->terminated;
            all_unproven += partition->unproven;
            all_faulty_power += partition->faulty_power;

            checkPartitionInvariants(runtime, partition);

            EXPECT_OUTCOME_TRUE(early_terminated_size,
                                partition->early_terminated.size());
            if (early_terminated_size > 0) {
              partitions_with_early_terminations.insert(part_id);
            }

            EXPECT_OUTCOME_TRUE(epochs, partition->expirations_epochs.keys());
            for (const auto &epoch : epochs) {
              EXPECT_EQ(quant.quantizeUp(epoch), epoch);
              expected_deadline_exp_queue[epoch].push_back(part_id);
            }

            return outcome::success();
          }};

      EXPECT_OUTCOME_TRUE_1(deadline.partitions.visit(partitions_visitor));

      EXPECT_EQ(deadline.live_sectors,
                all_sectors.size() - all_terminations.size());
      EXPECT_EQ(deadline.total_sectors, all_sectors.size());
      EXPECT_EQ(deadline.faulty_power, all_faulty_power);

      {
        for (const auto &[epoch, partitions] : expected_deadline_exp_queue) {
          EXPECT_OUTCOME_TRUE(bf, deadline.expirations_epochs.get(epoch));
          for (const auto &partition : partitions) {
            EXPECT_TRUE(bf.has(partition));
          }
        }
      }

      EXPECT_EQ(deadline.early_terminations,
                partitions_with_early_terminations);

      return std::make_tuple(all_sectors,
                             all_faults,
                             all_recoveries,
                             all_terminations,
                             all_unproven,
                             partition_sectors);
    }

    void checkPartitionInvariants(MockRuntime &runtime,
                                  const Universal<Partition> &partition) const {
      const auto live = partition->liveSectors();
      const auto active = partition->activeSectors();

      EXPECT_TRUE(live.contains(active));
      EXPECT_TRUE(live.contains(partition->faults));
      EXPECT_TRUE(live.contains(partition->unproven));
      EXPECT_FALSE(active.containsAny(partition->faults));
      EXPECT_FALSE(active.containsAny(partition->unproven));
      EXPECT_TRUE(partition->faults.contains(partition->recoveries));
      EXPECT_FALSE(live.containsAny(partition->terminated));
      EXPECT_FALSE(partition->faults.containsAny(partition->unproven));
      EXPECT_TRUE(partition->sectors.contains(partition->terminated));

      const auto live_sectors = selectSectorsTest(sectors, live);
      const auto live_power = powerForSectors(ssize, live_sectors);
      EXPECT_EQ(live_power, partition->live_power);

      const auto unproven_sectors =
          selectSectorsTest(sectors, partition->unproven);
      const auto unproven_power = powerForSectors(ssize, unproven_sectors);
      EXPECT_EQ(unproven_power, partition->unproven_power);

      const auto faulty_sectors = selectSectorsTest(sectors, partition->faults);
      const auto faulty_power = powerForSectors(ssize, faulty_sectors);
      EXPECT_EQ(faulty_power, partition->faulty_power);

      const auto recovering_sectors =
          selectSectorsTest(sectors, partition->recoveries);
      const auto recovering_power = powerForSectors(ssize, recovering_sectors);
      EXPECT_EQ(recovering_power, partition->recovering_power);

      const auto active_power = live_power - faulty_power - unproven_power;
      EXPECT_EQ(active_power, partition->activePower());

      {
        std::set<SectorNumber> seen_sectors;

        EXPECT_OUTCOME_TRUE(exp_q,
                            TypeManager::loadExpirationQueue(
                                runtime, partition->expirations_epochs, quant));

        auto visitor{[&](ChainEpoch epoch,
                         const ExpirationSet &es) -> outcome::result<void> {
          EXPECT_EQ(quant.quantizeUp(epoch), epoch);

          const auto all = es.on_time_sectors + es.early_sectors;
          const auto active = all - partition->faults;
          const auto faulty = all.intersect(partition->faults);

          const auto active_sectors = selectSectorsTest(live_sectors, active);
          const auto faulty_sectors = selectSectorsTest(live_sectors, faulty);
          const auto on_time_sectors =
              selectSectorsTest(live_sectors, es.on_time_sectors);
          const auto early_sectors =
              selectSectorsTest(live_sectors, es.early_sectors);

          EXPECT_TRUE(partition->faults.contains(es.early_sectors));
          EXPECT_TRUE(live.contains(es.on_time_sectors));

          for (const auto &sector : on_time_sectors) {
            const auto find = seen_sectors.find(sector.sector);
            EXPECT_TRUE(find == seen_sectors.end());
            seen_sectors.insert(sector.sector);

            const auto actual_epoch = quant.quantizeUp(sector.expiration);
            EXPECT_EQ(actual_epoch, epoch);
          }

          for (const auto &sector : early_sectors) {
            const auto find = seen_sectors.find(sector.sector);
            EXPECT_TRUE(find == seen_sectors.end());
            seen_sectors.insert(sector.sector);

            const auto actual_epoch = quant.quantizeUp(sector.expiration);
            EXPECT_TRUE(epoch < actual_epoch);
          }

          EXPECT_EQ(es.active_power, powerForSectors(ssize, active_sectors));
          EXPECT_EQ(es.faulty_power, powerForSectors(ssize, faulty_sectors));

          TokenAmount on_time_pledge{0};
          for (const auto &sector : on_time_sectors) {
            on_time_pledge += sector.init_pledge;
          }
          EXPECT_EQ(es.on_time_pledge, on_time_pledge);

          return outcome::success();
        }};

        EXPECT_OUTCOME_TRUE_1(exp_q->queue.visit(visitor));
      }

      {
        const BitfieldQueue<kEearlyTerminatedBitWidth> early_q{
            partition->early_terminated, kNoQuantization};
        RleBitset early_terms;

        auto visitor{[&](ChainEpoch epoch,
                         const RleBitset &bf) -> outcome::result<void> {
          for (const auto &i : bf) {
            const auto find = early_terms.find(i);
            EXPECT_TRUE(find == early_terms.end());
            early_terms.insert(i);
          }
          return outcome::success();
        }};

        EXPECT_OUTCOME_TRUE_1(early_q.queue.visit(visitor));

        EXPECT_TRUE(partition->terminated.contains(early_terms));
      }
    }
  };

  struct DeadlineTestV2 : testing::Test {
    void SetUp() override {
      actor_version = ActorVersion::kVersion2;
      ipld->actor_version = actor_version;
      cbor_blake::cbLoadT(ipld, deadline);

      EXPECT_CALL(runtime, getIpfsDatastore())
          .WillRepeatedly(testing::Invoke([&]() { return ipld; }));

      EXPECT_CALL(runtime, getActorVersion())
          .WillRepeatedly(testing::Invoke([&]() { return actor_version; }));

      sectors = {testSector(2, 1, 50, 60, 1000),
                 testSector(3, 2, 51, 61, 1001),
                 testSector(7, 3, 52, 62, 1002),
                 testSector(8, 4, 53, 63, 1003),
                 testSector(8, 5, 54, 64, 1004),
                 testSector(11, 6, 55, 65, 1005),
                 testSector(13, 7, 56, 66, 1006),
                 testSector(8, 8, 57, 67, 1007),
                 testSector(8, 9, 58, 68, 1008)};

      extra_sectors = {testSector(8, 10, 58, 68, 1008)};

      all_sectors = sectors;
      all_sectors.insert(
          all_sectors.end(), extra_sectors.begin(), extra_sectors.end());
    }

    void initExpectedDeadline() {
      expected_deadline = {};
      expected_deadline.quant = quant;
      expected_deadline.partition_size = partition_size;
      expected_deadline.ssize = ssize;
      expected_deadline.sectors = all_sectors;
    }

    Sectors sectorsArr(const std::vector<SectorOnChainInfo> &s) const {
      Sectors sectors_arr;
      cbor_blake::cbLoadT(ipld, sectors_arr);
      EXPECT_OUTCOME_TRUE_1(sectors_arr.store(s));
      return sectors_arr;
    }

    PowerPair sectorPower(const RleBitset &sector_nos) const {
      return powerForSectors(ssize, selectSectorsTest(all_sectors, sector_nos));
    }

    void addSectors(bool prove) {
      EXPECT_OUTCOME_TRUE(
          activated_power,
          deadline.addSectors(
              runtime, partition_size, false, sectors, ssize, quant));
      EXPECT_TRUE(activated_power.isZero());

      initExpectedDeadline();
      expected_deadline.unproven = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
      expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
      expected_deadline.partition_sectors.push_back({9});
      expected_deadline.assertDeadline(runtime, deadline);

      if (!prove) {
        return;
      }

      const std::vector<PoStPartition> post_partitions{
          {.index = 0, .skipped = {}},
          {.index = 1, .skipped = {}},
          {.index = 2, .skipped = {}}};

      EXPECT_OUTCOME_TRUE(
          result,
          deadline.recordProvenSectors(
              runtime, sectorsArr(sectors), ssize, quant, 0, post_partitions));
      EXPECT_EQ(result.power_delta, powerForSectors(ssize, sectors));

      EXPECT_OUTCOME_TRUE(process_result,
                          deadline.processDeadlineEnd(runtime, quant, 13));
      const auto &[new_faulty_power, failed_recovery_power] = process_result;
      EXPECT_TRUE(new_faulty_power.isZero());
      EXPECT_TRUE(failed_recovery_power.isZero());

      initExpectedDeadline();
      expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
      expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
      expected_deadline.partition_sectors.push_back({9});
      expected_deadline.assertDeadline(runtime, deadline);
    }

    void addThenTerminate(bool prove_first) {
      addSectors(prove_first);

      PartitionSectorMap sector_map;
      sector_map.map[0] = {1, 3};
      sector_map.map[1] = {6};

      EXPECT_OUTCOME_TRUE(
          removed_power,
          deadline.terminateSectors(
              runtime, sectorsArr(sectors), 15, sector_map, ssize, quant));

      const PowerPair expected_power =
          prove_first ? sectorPower({1, 3, 6}) : PowerPair();
      const RleBitset unproven =
          prove_first ? RleBitset{} : RleBitset{2, 4, 5, 7, 8, 9};

      EXPECT_EQ(removed_power, expected_power);

      initExpectedDeadline();
      expected_deadline.terminations = {1, 3, 6};
      expected_deadline.unproven = unproven;
      expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
      expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
      expected_deadline.partition_sectors.push_back({9});
      expected_deadline.assertDeadline(runtime, deadline);
    }

    void addThenTerminateThenPopEarly() {
      addThenTerminate(true);

      EXPECT_OUTCOME_TRUE(result,
                          deadline.popEarlyTerminations(runtime, 100, 100));
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
      expected_deadline.assertDeadline(runtime, deadline);
    }

    void addThenTerminateThenRemovePartition() {
      addThenTerminateThenPopEarly();

      EXPECT_OUTCOME_TRUE(result,
                          deadline.removePartitions(runtime, {0}, quant));
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
      expected_deadline.assertDeadline(runtime, deadline);
    }

    void addThenMarkFaulty(bool prove_first) {
      addSectors(prove_first);

      PartitionSectorMap sector_map;
      sector_map.map[0] = {1};
      sector_map.map[1] = {5, 6};

      EXPECT_OUTCOME_TRUE(
          power_delta,
          deadline.recordFaults(
              runtime, sectorsArr(sectors), ssize, quant, 9, sector_map));

      const PowerPair expected_power =
          prove_first ? sectorPower({1, 5, 6}) : PowerPair();
      const RleBitset unproven =
          prove_first ? RleBitset{} : RleBitset{2, 3, 4, 7, 8, 9};

      EXPECT_EQ(power_delta, expected_power.negative());

      initExpectedDeadline();
      expected_deadline.faults = {1, 5, 6};
      expected_deadline.unproven = unproven;
      expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
      expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
      expected_deadline.partition_sectors.push_back({9});
      expected_deadline.assertDeadline(runtime, deadline);
    }

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    ActorVersion actor_version;

    std::vector<SectorOnChainInfo> sectors;
    std::vector<SectorOnChainInfo> extra_sectors;
    std::vector<SectorOnChainInfo> all_sectors;
    SectorSize ssize{static_cast<uint64_t>(32) << 30};
    QuantSpec quant{4, 1};
    uint64_t partition_size{4};

    Deadline deadline;
    ExpectedDeadline expected_deadline;
  };

  TEST_F(DeadlineTestV2, AddsSectors) {
    addSectors(false);
  }

  TEST_F(DeadlineTestV2, AddsSectorsAndProves) {
    addSectors(true);
  }

  TEST_F(DeadlineTestV2, TerminatesSectors) {
    addThenTerminate(true);
  }

  TEST_F(DeadlineTestV2, TerminatesUnprovenSectors) {
    addThenTerminate(false);
  }

  TEST_F(DeadlineTestV2, PopsEarlyTerminations) {
    addThenTerminateThenPopEarly();
  }

  TEST_F(DeadlineTestV2, RemovesPartitions) {
    addThenTerminateThenRemovePartition();
  }

  TEST_F(DeadlineTestV2, MarksFaulty) {
    addThenMarkFaulty(true);
  }

  TEST_F(DeadlineTestV2, MarksUnprovenSectorsFaulty) {
    addThenMarkFaulty(false);
  }

  TEST_F(DeadlineTestV2, CannotRemovePartitionsWithEarlyTerminations) {
    addThenTerminate(false);

    const auto result = deadline.removePartitions(runtime, {0}, quant);
    EXPECT_EQ(result.error().message(),
              "cannot remove partitions from deadline with early terminations");
  }

  TEST_F(DeadlineTestV2, CanPopEarlyTerminationsInMultipleSteps) {
    addThenTerminate(true);

    TerminationResult result;

    EXPECT_OUTCOME_TRUE(pop_result1,
                        deadline.popEarlyTerminations(runtime, 2, 1));
    const auto &[result1, has_more1] = pop_result1;
    EXPECT_TRUE(has_more1);
    result.add(result1);

    EXPECT_OUTCOME_TRUE(pop_result2,
                        deadline.popEarlyTerminations(runtime, 2, 1));
    const auto &[result2, has_more2] = pop_result2;
    EXPECT_TRUE(has_more2);
    result.add(result2);

    EXPECT_OUTCOME_TRUE(pop_result3,
                        deadline.popEarlyTerminations(runtime, 1, 1));
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
    expected_deadline.assertDeadline(runtime, deadline);
  }

  TEST_F(DeadlineTestV2, CannotRemoveMissingPartition) {
    addThenTerminateThenRemovePartition();

    const auto result = deadline.removePartitions(runtime, {2}, quant);
    EXPECT_EQ(result.error().message(), "partition index is out of range");
  }

  TEST_F(DeadlineTestV2, RemovingNoPartitionsDoesNothing) {
    addThenTerminateThenPopEarly();

    EXPECT_OUTCOME_TRUE(result, deadline.removePartitions(runtime, {}, quant));
    const auto &[live, dead, removed_power] = result;

    EXPECT_TRUE(removed_power.isZero());
    EXPECT_TRUE(live.empty());
    EXPECT_TRUE(dead.empty());

    initExpectedDeadline();
    expected_deadline.terminations = {1, 3, 6};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(runtime, deadline);
  }

  TEST_F(DeadlineTestV2, FailsToRemovePartitionsWithFaultySectors) {
    addThenMarkFaulty(false);

    const auto result = deadline.removePartitions(runtime, {1}, quant);
    EXPECT_EQ(result.error().message(), "cannot remove, partition has faults");
  }

  TEST_F(DeadlineTestV2, TerminateProvenAndFaulty) {
    addThenMarkFaulty(true);  // 1, 5, 6 faulty

    PartitionSectorMap sector_map;
    sector_map.map[0] = {1, 3};
    sector_map.map[1] = {6};

    EXPECT_OUTCOME_TRUE(
        removed_power,
        deadline.terminateSectors(
            runtime, sectorsArr(sectors), 15, sector_map, ssize, quant));
    EXPECT_EQ(removed_power,
              powerForSectors(ssize, selectSectorsTest(sectors, {3})));

    initExpectedDeadline();
    expected_deadline.terminations = {1, 3, 6};
    expected_deadline.faults = {5};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(runtime, deadline);
  }

  TEST_F(DeadlineTestV2, TerminateUnprovenAndFaulty) {
    addThenMarkFaulty(false);  // 1, 5, 6 faulty

    PartitionSectorMap sector_map;
    sector_map.map[0] = {1, 3};
    sector_map.map[1] = {6};

    EXPECT_OUTCOME_TRUE(
        removed_power,
        deadline.terminateSectors(
            runtime, sectorsArr(sectors), 15, sector_map, ssize, quant));
    EXPECT_TRUE(removed_power.isZero());

    initExpectedDeadline();
    expected_deadline.terminations = {1, 3, 6};
    expected_deadline.unproven = {2, 4, 7, 8, 9};
    expected_deadline.faults = {5};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(runtime, deadline);
  }

  TEST_F(DeadlineTestV2, FailsToTerminateMissingSector) {
    addThenMarkFaulty(false);  // 1, 5, 6 faulty

    PartitionSectorMap sector_map;
    sector_map.map[0] = {6};

    const auto result = deadline.terminateSectors(
        runtime, sectorsArr(sectors), 15, sector_map, ssize, quant);
    EXPECT_EQ(result.error().message(), "can only terminate live sectors");
  }

  TEST_F(DeadlineTestV2, FailsToTerminateMissingPartition) {
    addThenMarkFaulty(false);  // 1, 5, 6 faulty

    PartitionSectorMap sector_map;
    sector_map.map[4] = {6};

    const auto result = deadline.terminateSectors(
        runtime, sectorsArr(sectors), 15, sector_map, ssize, quant);
    EXPECT_EQ(result.error().message(), "Not found");
  }

  TEST_F(DeadlineTestV2, FailsToTerminateAlreadyTerminatedSector) {
    addThenTerminate(false);  // 1, 3, 6 faulty

    PartitionSectorMap sector_map;
    sector_map.map[0] = {1, 2};

    const auto result = deadline.terminateSectors(
        runtime, sectorsArr(sectors), 15, sector_map, ssize, quant);
    EXPECT_EQ(result.error().message(), "can only terminate live sectors");
  }

  TEST_F(DeadlineTestV2, FaultySectorsExpire) {
    addThenMarkFaulty(true);

    EXPECT_OUTCOME_TRUE(exp, deadline.popExpiredSectors(runtime, 9, quant));

    const RleBitset expected_on_time{1, 2, 3, 4, 5, 8, 9};
    EXPECT_EQ(exp.on_time_sectors, expected_on_time);
    const RleBitset expected_early{6};
    EXPECT_EQ(exp.early_sectors, expected_early);

    initExpectedDeadline();
    expected_deadline.terminations = {1, 2, 3, 4, 5, 6, 8, 9};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(runtime, deadline);

    EXPECT_OUTCOME_TRUE(result,
                        deadline.popEarlyTerminations(runtime, 100, 100));
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
    expected_deadline.assertDeadline(runtime, deadline);
  }

  TEST_F(DeadlineTestV2, CannotPopExpiredSectorsBeforeProving) {
    addSectors(false);

    const auto result = deadline.popExpiredSectors(runtime, 9, quant);
    EXPECT_EQ(
        result.error().message(),
        "cannot pop expired sectors from a partition with unproven sectors");
  }

  TEST_F(DeadlineTestV2, PostAllTheThings) {
    addSectors(true);

    EXPECT_OUTCOME_TRUE(
        unproven_power_delta,
        deadline.addSectors(
            runtime, partition_size, false, extra_sectors, ssize, quant));
    EXPECT_TRUE(unproven_power_delta.isZero());

    const std::vector<PoStPartition> post_partitions1{
        {.index = 0, .skipped = {}}, {.index = 1, .skipped = {}}};

    EXPECT_OUTCOME_TRUE(post_result1,
                        deadline.recordProvenSectors(runtime,
                                                     sectorsArr(all_sectors),
                                                     ssize,
                                                     quant,
                                                     13,
                                                     post_partitions1));
    const RleBitset expected_sectors1{1, 2, 3, 4, 5, 6, 7, 8};
    EXPECT_EQ(post_result1.sectors, expected_sectors1);
    EXPECT_TRUE(post_result1.ignored_sectors.empty());
    EXPECT_TRUE(post_result1.new_faulty_power.isZero());
    EXPECT_TRUE(post_result1.retracted_recovery_power.isZero());
    EXPECT_TRUE(post_result1.recovered_power.isZero());

    initExpectedDeadline();
    expected_deadline.posts = {0, 1};
    expected_deadline.unproven = {10};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9, 10});
    expected_deadline.assertDeadline(runtime, deadline);

    const std::vector<PoStPartition> post_partitions2{
        {.index = 1, .skipped = {}}, {.index = 2, .skipped = {}}};

    EXPECT_OUTCOME_TRUE(post_result2,
                        deadline.recordProvenSectors(runtime,
                                                     sectorsArr(all_sectors),
                                                     ssize,
                                                     quant,
                                                     13,
                                                     post_partitions2));
    const RleBitset expected_sectors2{9, 10};
    EXPECT_EQ(post_result2.sectors, expected_sectors2);
    EXPECT_TRUE(post_result2.ignored_sectors.empty());
    EXPECT_TRUE(post_result2.new_faulty_power.isZero());
    EXPECT_TRUE(post_result2.retracted_recovery_power.isZero());
    EXPECT_TRUE(post_result2.recovered_power.isZero());
    EXPECT_EQ(post_result2.power_delta, sectorPower({10}));

    initExpectedDeadline();
    expected_deadline.posts = {0, 1, 2};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9, 10});
    expected_deadline.assertDeadline(runtime, deadline);

    EXPECT_OUTCOME_TRUE(process_result,
                        deadline.processDeadlineEnd(runtime, quant, 13));
    const auto &[power_delta, penalized_power] = process_result;
    EXPECT_TRUE(power_delta.isZero());
    EXPECT_TRUE(penalized_power.isZero());

    initExpectedDeadline();
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9, 10});
    expected_deadline.assertDeadline(runtime, deadline);
  }

  TEST_F(DeadlineTestV2,
         PostWithUnprovenFaultsRecoveriesAndRetractedRecoveries) {
    addThenMarkFaulty(true);

    EXPECT_OUTCOME_TRUE(
        unproven_power_delta,
        deadline.addSectors(
            runtime, partition_size, false, extra_sectors, ssize, quant));
    EXPECT_TRUE(unproven_power_delta.isZero());

    PartitionSectorMap sector_map;
    sector_map.map[0] = {1};
    sector_map.map[1] = {6};

    EXPECT_OUTCOME_TRUE_1(deadline.declareFaultsRecovered(
        sectorsArr(all_sectors), ssize, sector_map));

    initExpectedDeadline();
    expected_deadline.recovering = {1, 6};
    expected_deadline.faults = {1, 5, 6};
    expected_deadline.unproven = {10};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9, 10});
    expected_deadline.assertDeadline(runtime, deadline);

    const std::vector<PoStPartition> post_partitions{
        {.index = 0, .skipped = {1}}, {.index = 1, .skipped = {7}}};

    EXPECT_OUTCOME_TRUE(post_result,
                        deadline.recordProvenSectors(runtime,
                                                     sectorsArr(all_sectors),
                                                     ssize,
                                                     quant,
                                                     13,
                                                     post_partitions));
    const RleBitset expected_sectors{1, 2, 3, 4, 5, 6, 7, 8};
    EXPECT_EQ(post_result.sectors, expected_sectors);
    const RleBitset expected_ignored{1, 5, 7};
    EXPECT_EQ(post_result.ignored_sectors, expected_ignored);
    EXPECT_EQ(post_result.new_faulty_power, sectorPower({7}));
    EXPECT_EQ(post_result.retracted_recovery_power, sectorPower({1}));
    EXPECT_EQ(post_result.recovered_power, sectorPower({6}));
    EXPECT_TRUE(post_result.power_delta.isZero());

    initExpectedDeadline();
    expected_deadline.posts = {0, 1};
    expected_deadline.faults = {1, 5, 7};
    expected_deadline.unproven = {10};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9, 10});
    expected_deadline.assertDeadline(runtime, deadline);

    EXPECT_OUTCOME_TRUE(process_result,
                        deadline.processDeadlineEnd(runtime, quant, 13));
    const auto &[power_delta, penalized_power] = process_result;

    EXPECT_EQ(power_delta, sectorPower({9}).negative());
    EXPECT_EQ(penalized_power, sectorPower({9, 10}));

    initExpectedDeadline();
    expected_deadline.faults = {1, 5, 7, 9, 10};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9, 10});
    expected_deadline.assertDeadline(runtime, deadline);
  }

  TEST_F(DeadlineTestV2, PostWithSkippedUnproven) {
    addSectors(true);

    EXPECT_OUTCOME_TRUE(
        unproven_power_delta,
        deadline.addSectors(
            runtime, partition_size, false, extra_sectors, ssize, quant));
    EXPECT_TRUE(unproven_power_delta.isZero());

    const std::vector<PoStPartition> post_partitions{
        {.index = 0, .skipped = {}},
        {.index = 1, .skipped = {}},
        {.index = 2, .skipped = {10}}};

    EXPECT_OUTCOME_TRUE(post_result,
                        deadline.recordProvenSectors(runtime,
                                                     sectorsArr(all_sectors),
                                                     ssize,
                                                     quant,
                                                     13,
                                                     post_partitions));
    const RleBitset expected_sectors{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    EXPECT_EQ(post_result.sectors, expected_sectors);
    const RleBitset expected_ignored{10};
    EXPECT_EQ(post_result.ignored_sectors, expected_ignored);
    EXPECT_EQ(post_result.new_faulty_power, sectorPower({10}));
    EXPECT_TRUE(post_result.power_delta.isZero());  // not proven yet
    EXPECT_TRUE(post_result.retracted_recovery_power.isZero());
    EXPECT_TRUE(post_result.recovered_power.isZero());

    initExpectedDeadline();
    expected_deadline.posts = {0, 1, 2};
    expected_deadline.faults = {10};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9, 10});
    expected_deadline.assertDeadline(runtime, deadline);

    EXPECT_OUTCOME_TRUE(process_result,
                        deadline.processDeadlineEnd(runtime, quant, 13));
    const auto &[power_delta, penalized_power] = process_result;

    EXPECT_TRUE(power_delta.isZero());
    EXPECT_TRUE(penalized_power.isZero());

    initExpectedDeadline();
    expected_deadline.faults = {10};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9, 10});
    expected_deadline.assertDeadline(runtime, deadline);
  }

  TEST_F(DeadlineTestV2, PostMissingPartition) {
    addSectors(true);

    EXPECT_OUTCOME_TRUE(
        unproven_power_delta,
        deadline.addSectors(
            runtime, partition_size, false, extra_sectors, ssize, quant));
    EXPECT_TRUE(unproven_power_delta.isZero());

    const std::vector<PoStPartition> post_partitions{
        {.index = 0, .skipped = {}}, {.index = 3, .skipped = {}}};

    const auto result = deadline.recordProvenSectors(
        runtime, sectorsArr(all_sectors), ssize, quant, 13, post_partitions);
    EXPECT_EQ(result.error().message(), "Not found");
  }

  TEST_F(DeadlineTestV2, RetractRecoveries) {
    addThenMarkFaulty(true);

    PartitionSectorMap sector_map1;
    sector_map1.map[0] = {1};
    sector_map1.map[1] = {6};

    EXPECT_OUTCOME_TRUE_1(deadline.declareFaultsRecovered(
        sectorsArr(sectors), ssize, sector_map1));

    PartitionSectorMap sector_map2;
    sector_map2.map[0] = {1};

    EXPECT_OUTCOME_TRUE(
        power_delta,
        deadline.recordFaults(
            runtime, sectorsArr(sectors), ssize, quant, 13, sector_map2));
    EXPECT_TRUE(power_delta.isZero());

    initExpectedDeadline();
    expected_deadline.recovering = {6};
    expected_deadline.faults = {1, 5, 6};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(runtime, deadline);

    const std::vector<PoStPartition> post_partitions{
        {.index = 0, .skipped = {}},
        {.index = 1, .skipped = {}},
        {.index = 2, .skipped = {}}};

    EXPECT_OUTCOME_TRUE(
        post_result,
        deadline.recordProvenSectors(
            runtime, sectorsArr(sectors), ssize, quant, 13, post_partitions));
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
    expected_deadline.assertDeadline(runtime, deadline);

    EXPECT_OUTCOME_TRUE(process_result,
                        deadline.processDeadlineEnd(runtime, quant, 13));
    const auto &[new_faulty_power, failed_recovery_power] = process_result;
    EXPECT_TRUE(new_faulty_power.isZero());
    EXPECT_TRUE(failed_recovery_power.isZero());

    initExpectedDeadline();
    expected_deadline.faults = {1, 5};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(runtime, deadline);
  }

  TEST_F(DeadlineTestV2, RescheduleExpirations) {
    addThenMarkFaulty(true);

    PartitionSectorMap sector_map;
    sector_map.map[1] = {6, 7, 99};  // 99 should be skipped, it doesn't exist.
    sector_map.map[5] = {100};       // partition 5 doesn't exist.
    sector_map.map[2] = {};          // empty bitfield should be fine.

    EXPECT_OUTCOME_TRUE(
        replaced,
        deadline.rescheduleSectorExpirations(
            runtime, sectorsArr(sectors), 1, sector_map, ssize, quant));
    EXPECT_EQ(replaced.size(), 1);

    EXPECT_OUTCOME_TRUE(exp, deadline.popExpiredSectors(runtime, 1, quant));

    const auto sector7 = selectSectorsTest(sectors, {7})[0];

    initExpectedDeadline();
    expected_deadline.faults = {1, 5, 6};
    expected_deadline.terminations = {7};
    expected_deadline.partition_sectors.push_back({1, 2, 3, 4});
    expected_deadline.partition_sectors.push_back({5, 6, 7, 8});
    expected_deadline.partition_sectors.push_back({9});
    expected_deadline.assertDeadline(runtime, deadline);

    EXPECT_EQ(exp.active_power,
              PowerPair(ssize, qaPowerForSector(ssize, sector7)));
    EXPECT_TRUE(exp.faulty_power.isZero());
    EXPECT_EQ(exp.on_time_pledge, sector7.init_pledge);
  }

  TEST_F(DeadlineTestV2, CannotDeclareFaultsInMissingPartitions) {
    addSectors(true);

    PartitionSectorMap sector_map;
    sector_map.map[0] = {1};
    sector_map.map[4] = {6};

    const auto result = deadline.recordFaults(
        runtime, sectorsArr(sectors), ssize, quant, 17, sector_map);
    EXPECT_EQ(result.error().message(), "Not found");
  }

  TEST_F(DeadlineTestV2, CannotDeclareFaultsRecoveredInMissingPartitions) {
    addThenMarkFaulty(true);

    PartitionSectorMap sector_map;
    sector_map.map[0] = {1};
    sector_map.map[4] = {6};

    const auto result =
        deadline.declareFaultsRecovered(sectorsArr(sectors), ssize, sector_map);
    EXPECT_EQ(result.error().message(), "Not found");
  }

}  // namespace fc::vm::actor::builtin::v2::miner
