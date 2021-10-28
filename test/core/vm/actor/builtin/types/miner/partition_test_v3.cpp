/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v3/partition.hpp"

#include <gtest/gtest.h>
#include "common/container_utils.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "test_utils.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/version.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using common::slice;
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::SectorSize;
  using primitives::sector::getSealProofWindowPoStPartitionSectors;
  using primitives::sector::getSectorSize;
  using primitives::sector::RegisteredSealProof;
  using runtime::MockRuntime;
  using storage::ipfs::InMemoryDatastore;
  using types::miner::BitfieldQueue;
  using types::miner::ExpirationSet;
  using types::miner::kEearlyTerminatedBitWidth;
  using types::miner::kNoQuantization;
  using types::miner::powerForSectors;
  using types::miner::PowerPair;
  using types::miner::qaPowerForSector;
  using types::miner::QuantSpec;
  using types::miner::SectorOnChainInfo;
  using types::miner::Sectors;

  struct ExpectExpirationGroup {
    ChainEpoch expiration{};
    RleBitset sectors;
  };

  struct PartitionTestV3 : testing::Test {
    void SetUp() override {
      actor_version = ActorVersion::kVersion2;
      ipld->actor_version = actor_version;
      cbor_blake::cbLoadT(ipld, partition);

      EXPECT_CALL(runtime, getIpfsDatastore())
          .WillRepeatedly(testing::Invoke([&]() { return ipld; }));

      EXPECT_CALL(runtime, getActorVersion())
          .WillRepeatedly(testing::Invoke([&]() { return actor_version; }));
    }

    void setupUnproven() {
      sectors = {testSector(2, 1, 50, 60, 1000),
                 testSector(3, 2, 51, 61, 1001),
                 testSector(7, 3, 52, 62, 1002),
                 testSector(8, 4, 53, 63, 1003),
                 testSector(11, 5, 54, 64, 1004),
                 testSector(13, 6, 55, 65, 1005)};

      EXPECT_OUTCOME_TRUE(power,
                          partition.addSectors(false, sectors, ssize, quant));

      EXPECT_EQ(power, powerForSectors(ssize, sectors));
    }

    void setup() {
      setupUnproven();

      const auto power = partition.activateUnproven();
      EXPECT_EQ(power, powerForSectors(ssize, sectors));
    }

    void setupProven() {
      sectors = {testSector(2, 1, 50, 60, 1000),
                 testSector(3, 2, 51, 61, 1001),
                 testSector(7, 3, 52, 62, 1002),
                 testSector(8, 4, 53, 63, 1003),
                 testSector(11, 5, 54, 64, 1004),
                 testSector(13, 6, 55, 65, 1005)};

      EXPECT_OUTCOME_TRUE(power,
                          partition.addSectors(true, sectors, ssize, quant));
      EXPECT_EQ(power, powerForSectors(ssize, sectors));
    }

    void assertPartitionExpirationQueue() const {
      auto queue = loadExpirationQueue(partition.expirations_epochs, quant);

      for (const auto &group : groups) {
        requireNoExpirationGroupsBefore(group.expiration, *queue);
        EXPECT_OUTCOME_TRUE(es, queue->popUntil(group.expiration));

        const auto all_sectors = es.on_time_sectors + es.early_sectors;
        EXPECT_EQ(group.sectors, all_sectors);
      }
    }

    void checkPartitionInvariants() const {
      const auto live = partition.liveSectors();
      const auto active = partition.activeSectors();

      EXPECT_TRUE(live.contains(active));
      EXPECT_TRUE(live.contains(partition.faults));
      EXPECT_TRUE(live.contains(partition.unproven));
      EXPECT_FALSE(active.containsAny(partition.faults));
      EXPECT_FALSE(active.containsAny(partition.unproven));
      EXPECT_TRUE(partition.faults.contains(partition.recoveries));
      EXPECT_FALSE(live.containsAny(partition.terminated));
      EXPECT_FALSE(partition.faults.containsAny(partition.unproven));
      EXPECT_TRUE(partition.sectors.contains(partition.terminated));

      const auto live_sectors = selectSectorsTest(sectors, live);
      const auto live_power = powerForSectors(ssize, live_sectors);
      EXPECT_EQ(live_power, partition.live_power);

      const auto unproven_sectors =
          selectSectorsTest(sectors, partition.unproven);
      const auto unproven_power = powerForSectors(ssize, unproven_sectors);
      EXPECT_EQ(unproven_power, partition.unproven_power);

      const auto faulty_sectors = selectSectorsTest(sectors, partition.faults);
      const auto faulty_power = powerForSectors(ssize, faulty_sectors);
      EXPECT_EQ(faulty_power, partition.faulty_power);

      const auto recovering_sectors =
          selectSectorsTest(sectors, partition.recoveries);
      const auto recovering_power = powerForSectors(ssize, recovering_sectors);
      EXPECT_EQ(recovering_power, partition.recovering_power);

      const auto active_power = live_power - faulty_power - unproven_power;
      EXPECT_EQ(active_power, partition.activePower());

      {
        std::set<SectorNumber> seen_sectors;

        auto exp_q = loadExpirationQueue(partition.expirations_epochs, quant);

        auto visitor{[&](ChainEpoch epoch,
                         const ExpirationSet &es) -> outcome::result<void> {
          EXPECT_EQ(quant.quantizeUp(epoch), epoch);

          const auto all = es.on_time_sectors + es.early_sectors;
          const auto active = all - partition.faults;
          const auto faulty = all.intersect(partition.faults);

          const auto active_sectors = selectSectorsTest(live_sectors, active);
          const auto faulty_sectors = selectSectorsTest(live_sectors, faulty);
          const auto on_time_sectors =
              selectSectorsTest(live_sectors, es.on_time_sectors);
          const auto early_sectors =
              selectSectorsTest(live_sectors, es.early_sectors);

          EXPECT_TRUE(partition.faults.contains(es.early_sectors));
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
            partition.early_terminated, kNoQuantization};
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

        EXPECT_TRUE(partition.terminated.contains(early_terms));
      }
    }

    void assertPartitionState(const RleBitset &all_sectors_ids,
                              const RleBitset &faults,
                              const RleBitset &recovering,
                              const RleBitset &terminations,
                              const RleBitset &unproven) const {
      EXPECT_EQ(partition.faults, faults);
      EXPECT_EQ(partition.recoveries, recovering);
      EXPECT_EQ(partition.terminated, terminations);
      EXPECT_EQ(partition.unproven, unproven);
      EXPECT_EQ(partition.sectors, all_sectors_ids);

      checkPartitionInvariants();
    }

    Sectors getSectorsArray() const {
      Sectors sectors_arr;
      cbor_blake::cbLoadT(ipld, sectors_arr);

      EXPECT_OUTCOME_TRUE_1(sectors_arr.store(sectors));
      return sectors_arr;
    }

    std::vector<SectorOnChainInfo> rescheduleSectors(
        ChainEpoch target, const RleBitset &filter) const {
      std::vector<SectorOnChainInfo> output;

      for (const auto &sector : sectors) {
        auto sector_copy = sector;
        if (filter.has(sector_copy.sector)) {
          sector_copy.expiration = target;
        }
        output.push_back(sector_copy);
      }
      return output;
    }

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    ActorVersion actor_version;

    std::vector<SectorOnChainInfo> sectors;
    SectorSize ssize{static_cast<uint64_t>(32) << 30};
    QuantSpec quant{4, 1};
    ChainEpoch exp{100};

    Partition partition;
    std::vector<ExpectExpirationGroup> groups;
  };

  /**
   * @given partition with sectors
   * @when check partition state
   * @then partition is correct
   */
  TEST_F(PartitionTestV3, AddsSectorsAndReportsSectorStats) {
    setup();

    assertPartitionState({1, 2, 3, 4, 5, 6}, {}, {}, {}, {});

    groups.push_back({5, {1, 2}});
    groups.push_back({9, {3, 4}});
    groups.push_back({13, {5, 6}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors
   * @when add already existed sector
   * @then return error
   */
  TEST_F(PartitionTestV3, DoesntAddSectorsTwice) {
    setup();

    const auto result =
        partition.addSectors(false, slice(sectors, 0, 1), ssize, quant);
    EXPECT_EQ(result.error().message(), "not all added sectors are new");
  }

  /**
   * @given partition with sectors
   * @when add some correct faults are not proven
   * @then partition is correct
   */
  TEST_F(PartitionTestV3, AddsFaultsNotProven) {
    setupUnproven();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5};
    PowerPair power_delta;
    PowerPair new_faulty_power;
    EXPECT_OUTCOME_TRUE(
        result,
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));
    std::tie(std::ignore, power_delta, new_faulty_power) = result;

    EXPECT_EQ(new_faulty_power,
              powerForSectors(ssize, selectSectorsTest(sectors, fault_set)));
    EXPECT_EQ(power_delta, PowerPair());

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5}, {}, {}, {1, 2, 3, 6});

    groups.push_back({5, {1, 2}});
    groups.push_back({9, {3, 4, 5}});
    groups.push_back({13, {6}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors
   * @when add some correct faults are proven
   * @then partition is correct
   */
  TEST_F(PartitionTestV3, AddsFaultsProven) {
    setupUnproven();
    partition.activateUnproven();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5};
    PowerPair power_delta;
    PowerPair new_faulty_power;
    EXPECT_OUTCOME_TRUE(
        result,
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));
    std::tie(std::ignore, power_delta, new_faulty_power) = result;

    const auto expected_faulty_power =
        powerForSectors(ssize, selectSectorsTest(sectors, fault_set));
    EXPECT_EQ(new_faulty_power, expected_faulty_power);
    EXPECT_EQ(power_delta, expected_faulty_power.negative());

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5}, {}, {}, {});

    groups.push_back({5, {1, 2}});
    groups.push_back({9, {3, 4, 5}});
    groups.push_back({13, {6}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors
   * @when add some correct faults twice
   * @then partition is correct
   */
  TEST_F(PartitionTestV3, ReAddingFaultsIsANoOp) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set1{4, 5};
    PowerPair power_delta1;
    PowerPair new_faulty_power1;
    EXPECT_OUTCOME_TRUE(
        result1,
        partition.recordFaults(sectors_arr, fault_set1, 7, ssize, quant));
    std::tie(std::ignore, power_delta1, new_faulty_power1) = result1;

    const auto expected_faulty_power1 =
        powerForSectors(ssize, selectSectorsTest(sectors, fault_set1));
    EXPECT_EQ(new_faulty_power1, expected_faulty_power1);
    EXPECT_EQ(power_delta1, expected_faulty_power1.negative());

    const RleBitset fault_set2{5, 6};
    EXPECT_OUTCOME_TRUE(
        result2,
        partition.recordFaults(sectors_arr, fault_set2, 3, ssize, quant));
    const auto &[new_faults2, power_delta2, new_faulty_power2] = result2;

    const RleBitset expected_new_faults{6};
    EXPECT_EQ(new_faults2, expected_new_faults);
    const auto expected_faulty_power2 =
        powerForSectors(ssize, selectSectorsTest(sectors, {6}));
    EXPECT_EQ(new_faulty_power2, expected_faulty_power2);
    EXPECT_EQ(power_delta2, expected_faulty_power2.negative());

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {}, {}, {});

    groups.push_back({5, {1, 2, 6}});
    groups.push_back({9, {3, 4, 5}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors
   * @when add fault for missing sector
   * @then return error
   */
  TEST_F(PartitionTestV3, FailsToAddFaultsForMissingSectors) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{99};
    const auto result =
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant);
    EXPECT_EQ(result.error().message(), "failed fault declaration");
  }

  /**
   * @given partition with sectors
   * @when add some correct recoveries for faults
   * @then partition is correct
   */
  TEST_F(PartitionTestV3, AddsRecoveries) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {4, 5}, {}, {});
  }

  /**
   * @given partition with sectors
   * @when declare some faults to recover and then add these faults again
   * @then partition is correct, recoveries don't contain faults added again
   */
  TEST_F(PartitionTestV3, RemoveRecoveries) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    RleBitset new_faults1;
    EXPECT_OUTCOME_TRUE(
        result1, partition.recordFaults(sectors_arr, {}, 7, ssize, quant));
    std::tie(new_faults1, std::ignore, std::ignore) = result1;
    EXPECT_TRUE(new_faults1.empty());

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {4, 5}, {}, {});

    RleBitset new_faults2;
    EXPECT_OUTCOME_TRUE(
        result2, partition.recordFaults(sectors_arr, {5}, 10, ssize, quant));
    std::tie(new_faults2, std::ignore, std::ignore) = result2;
    EXPECT_TRUE(new_faults2.empty());

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {4}, {}, {});
  }

  /**
   * @given partition with sectors and faults
   * @when recover faults
   * @then partition is correct and doesn't contain these faults anymore
   */
  TEST_F(PartitionTestV3, RecoversFaults) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    EXPECT_OUTCOME_TRUE(recovered_power,
                        partition.recoverFaults(sectors_arr, ssize, quant));
    EXPECT_EQ(recovered_power,
              powerForSectors(ssize, selectSectorsTest(sectors, recover_set)));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {6}, {}, {}, {});

    groups.push_back({5, {1, 2}});
    groups.push_back({9, {3, 4, 6}});
    groups.push_back({13, {5}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors and faults
   * @when declare some intersected faults to recover twice
   * @then partition is correct, recoveries are not duplicated
   */
  TEST_F(PartitionTestV3, FaultyPowerRecoveredExactlyOnce) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{3, 4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, fault_set));

    EXPECT_EQ(partition.recovering_power,
              powerForSectors(ssize, selectSectorsTest(sectors, fault_set)));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {4, 5, 6}, {}, {});
  }

  /**
   * @given partition with sectors
   * @when recover non-existed fault
   * @then return error
   */
  TEST_F(PartitionTestV3, MissingSectorsAreNotRecovered) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{99};
    const auto result =
        partition.declareFaultsRecovered(sectors_arr, ssize, fault_set);
    EXPECT_EQ(result.error().message(), "failed fault declaration");
  }

  /**
   * @given partition with sectors
   * @when reschedule expirations
   * @then partition is correct, expirations are rescheduled ecxept faults
   */
  TEST_F(PartitionTestV3, ReschedulesExpirations) {
    setup();
    const auto unproven_sector = testSector(13, 7, 55, 65, 1006);
    sectors.push_back(unproven_sector);
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{2};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));

    EXPECT_OUTCOME_TRUE(
        power_delta,
        partition.addSectors(false, {unproven_sector}, ssize, quant));
    EXPECT_EQ(power_delta, powerForSectors(ssize, {unproven_sector}));

    EXPECT_OUTCOME_TRUE(moved,
                        partition.rescheduleExpirationsV2(
                            sectors_arr, 18, {2, 4, 6, 7}, ssize, quant));

    EXPECT_EQ(moved.size(), 3);
    std::sort(moved.begin(), moved.end(), [](auto lhs, auto rhs) {
      return lhs.sector < rhs.sector;
    });
    EXPECT_EQ(moved[0].sector, 4);
    EXPECT_EQ(moved[1].sector, 6);
    EXPECT_EQ(moved[2].sector, 7);

    sectors = rescheduleSectors(18, {4, 6, 7});

    assertPartitionState({1, 2, 3, 4, 5, 6, 7}, {2}, {}, {}, {7});

    groups.push_back({5, {1, 2}});
    groups.push_back({9, {3}});
    groups.push_back({13, {5}});
    groups.push_back({21, {4, 6, 7}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors
   * @when replace some sectors
   * @then partition is correct, sectors are replaced
   */
  TEST_F(PartitionTestV3, ReplaceSectors) {
    setup();

    const auto old_sectors = slice(sectors, 1, 4);
    const auto old_sector_power = powerForSectors(ssize, old_sectors);
    const TokenAmount old_sector_pledge = 1001 + 1002 + 1003;

    const std::vector<SectorOnChainInfo> new_sectors = {
        testSector(10, 2, 150, 260, 3000),
        testSector(10, 7, 151, 261, 3001),
        testSector(18, 8, 152, 262, 3002)};
    const auto new_sector_power = powerForSectors(ssize, new_sectors);
    const TokenAmount new_sector_pledge = 3000 + 3001 + 3002;

    EXPECT_OUTCOME_TRUE(
        result,
        partition.replaceSectors(old_sectors, new_sectors, ssize, quant));
    const auto &[power_delta, pledge_delta] = result;

    EXPECT_EQ(power_delta, new_sector_power - old_sector_power);
    EXPECT_EQ(pledge_delta, new_sector_pledge - old_sector_pledge);

    auto all_sectors = new_sectors;
    all_sectors.insert(all_sectors.end(), sectors.begin(), sectors.begin() + 1);
    all_sectors.insert(all_sectors.end(), sectors.begin() + 4, sectors.end());

    sectors = all_sectors;

    assertPartitionState({1, 2, 5, 6, 7, 8}, {}, {}, {}, {});

    groups.push_back({5, {1}});
    groups.push_back({13, {2, 5, 6, 7}});
    groups.push_back({21, {8}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors
   * @when replace fault sector
   * @then return error
   */
  TEST_F(PartitionTestV3,
         ReplaceSectorsErrorsWhenAttemptingToReplaceInactiveSector) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{2};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));

    const auto old_sectors = slice(sectors, 1, 4);
    const std::vector<SectorOnChainInfo> new_sectors = {
        testSector(10, 2, 150, 260, 3000)};

    const auto result =
        partition.replaceSectors(old_sectors, new_sectors, ssize, quant);
    EXPECT_EQ(result.error().message(), "refusing to replace inactive sectors");
  }

  /**
   * @given partition with unproven sectors
   * @when replace unproven sectors
   * @then return error
   */
  TEST_F(PartitionTestV3, ReplaceSectorsErrorsWhenAttemptingToUnprovenSector) {
    setupUnproven();

    const auto old_sectors = slice(sectors, 1, 4);
    std::vector<SectorOnChainInfo> new_sectors = {
        testSector(10, 2, 150, 260, 3000)};

    const auto result =
        partition.replaceSectors(old_sectors, new_sectors, ssize, quant);
    EXPECT_EQ(result.error().message(), "refusing to replace inactive sectors");
  }

  /**
   * @given partition with sectors
   * @when terminate some sectors (include faults and recoveries)
   * @then partition is correct, sectors are terminated
   */
  TEST_F(PartitionTestV3, TerminateSectors) {
    setup();
    const auto unproven_sector = testSector(13, 7, 55, 65, 1006);
    sectors.push_back(unproven_sector);
    const auto sectors_arr = getSectorsArray();

    EXPECT_OUTCOME_TRUE(
        power_delta,
        partition.addSectors(false, {unproven_sector}, ssize, quant));
    EXPECT_EQ(power_delta, powerForSectors(ssize, {unproven_sector}));

    const RleBitset fault_set{3, 4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    const RleBitset terminations{1, 3, 5, 7};
    const ChainEpoch termination_epoch{3};
    EXPECT_OUTCOME_TRUE(
        removed,
        partition.terminateSectors(
            sectors_arr, termination_epoch, terminations, ssize, quant));

    EXPECT_EQ(removed.active_power,
              powerForSectors(ssize, selectSectorsTest(sectors, {1})));
    EXPECT_EQ(removed.faulty_power,
              powerForSectors(ssize, selectSectorsTest(sectors, {3, 5})));

    assertPartitionState({1, 2, 3, 4, 5, 6, 7}, {4, 6}, {4}, terminations, {});

    groups.push_back({5, {2}});
    groups.push_back({9, {4, 6}});
    assertPartitionExpirationQueue();

    const BitfieldQueue<kEearlyTerminatedBitWidth> queue{
        partition.early_terminated, kNoQuantization};
    EXPECT_OUTCOME_EQ(queue.queue.size(), 1);
    EXPECT_OUTCOME_TRUE(terminated, queue.queue.get(termination_epoch));
    EXPECT_EQ(terminated, terminations);
  }

  /**
   * @given partition with sectors
   * @when terminate non-existed sector
   * @then return error
   */
  TEST_F(PartitionTestV3, TerminateNonExistentSectors) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const auto result =
        partition.terminateSectors(sectors_arr, 3, {99}, ssize, quant);
    EXPECT_EQ(result.error().message(), "can only terminate live sectors");
  }

  /**
   * @given partition with sectors
   * @when terminate already terminated sectors
   * @then return error
   */
  TEST_F(PartitionTestV3, TerminateAlreadyTerminatedSector) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset terminations{1};
    const ChainEpoch termination_epoch{3};
    EXPECT_OUTCOME_TRUE(
        removed,
        partition.terminateSectors(
            sectors_arr, termination_epoch, terminations, ssize, quant));
    EXPECT_EQ(removed.active_power,
              powerForSectors(ssize, selectSectorsTest(sectors, {1})));
    EXPECT_EQ(removed.faulty_power, PowerPair());
    EXPECT_EQ(removed.count(), 1);

    const auto result = partition.terminateSectors(
        sectors_arr, termination_epoch, terminations, ssize, quant);
    EXPECT_EQ(result.error().message(), "can only terminate live sectors");
  }

  /**
   * @given partition with sectors
   * @when add faults sectors are already terminated
   * @then partition is correct, sectors are moved to faults from termination
   */
  TEST_F(PartitionTestV3, MarkTerminatedSectorsAsFaulty) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset terminations{1};
    const ChainEpoch termination_epoch{3};
    EXPECT_OUTCOME_TRUE_1(partition.terminateSectors(
        sectors_arr, termination_epoch, terminations, ssize, quant));

    RleBitset new_faults;
    EXPECT_OUTCOME_TRUE(
        result,
        partition.recordFaults(sectors_arr, terminations, 5, ssize, quant));
    std::tie(new_faults, std::ignore, std::ignore) = result;
    EXPECT_TRUE(new_faults.empty());
  }

  /**
   * @given partition with sectors
   * @when pop expired sectors
   * @then partition is correct
   */
  TEST_F(PartitionTestV3, PopExpiringSectors) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 2, ssize, quant));

    const ChainEpoch expire_epoch{5};
    EXPECT_OUTCOME_TRUE(exp_set,
                        partition.popExpiredSectors(expire_epoch, quant));

    const RleBitset expected_on_time_sectors{1, 2};
    EXPECT_EQ(exp_set.on_time_sectors, expected_on_time_sectors);

    const RleBitset expected_early_sectors{4};
    EXPECT_EQ(exp_set.early_sectors, expected_early_sectors);

    EXPECT_EQ(exp_set.on_time_pledge, 1000 + 1001);

    EXPECT_EQ(exp_set.active_power,
              powerForSectors(ssize, slice(sectors, 0, 2)));

    EXPECT_EQ(exp_set.faulty_power,
              powerForSectors(ssize, slice(sectors, 3, 4)));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {}, {}, {1, 2, 4}, {});

    groups.push_back({9, {3}});
    groups.push_back({13, {5, 6}});
    assertPartitionExpirationQueue();

    const BitfieldQueue<kEearlyTerminatedBitWidth> queue{
        partition.early_terminated, kNoQuantization};
    EXPECT_OUTCOME_EQ(queue.queue.size(), 1);
    EXPECT_OUTCOME_TRUE(expired, queue.queue.get(expire_epoch));
    EXPECT_EQ(expired, fault_set);
  }

  /**
   * @given partition with sectors
   * @when pop recovered sector
   * @then return error
   */
  TEST_F(PartitionTestV3, PopExpiringSectorsErrorsIfARecoveryExists) {
    setup();
    const auto sectors_arr = getSectorsArray();

    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, {5}, 2, ssize, quant));
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, {5}));

    const auto result = partition.popExpiredSectors(5, quant);
    EXPECT_EQ(result.error().message(),
              "unexpected recoveries while processing expirations");
  }

  /**
   * @given partition with unproven sectors
   * @when pop unproven expired sectors
   * @then return error
   */
  TEST_F(PartitionTestV3, PopExpiringSectorsErrorsIfUnprovenSectorsExist) {
    setupUnproven();

    const auto result = partition.popExpiredSectors(5, quant);
    EXPECT_EQ(
        result.error().message(),
        "cannot pop expired sectors from a partition with unproven sectors");
  }

  /**
   * @given partition with sectors
   * @when record missing post
   * @then partition is correct
   */
  TEST_F(PartitionTestV3, RecordsMissingPost) {
    setup();
    const auto unproven_sector = testSector(13, 7, 55, 65, 1006);
    sectors.push_back(unproven_sector);
    const auto sectors_arr = getSectorsArray();

    EXPECT_OUTCOME_TRUE(
        power, partition.addSectors(false, {unproven_sector}, ssize, quant));
    EXPECT_EQ(power, powerForSectors(ssize, {unproven_sector}));

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    EXPECT_OUTCOME_TRUE(result, partition.recordMissedPostV2(6, quant));
    const auto &[power_delta, penalized_power, new_faulty_power] = result;

    auto faulty_sectors = slice(sectors, 0, 3);
    faulty_sectors.push_back(sectors[6]);
    EXPECT_EQ(new_faulty_power, powerForSectors(ssize, faulty_sectors));
    EXPECT_EQ(penalized_power,
              powerForSectors(ssize, sectors)
                  - PowerPair(ssize, qaPowerForSector(ssize, sectors[5])));
    EXPECT_EQ(power_delta,
              powerForSectors(ssize, slice(sectors, 0, 3)).negative());

    assertPartitionState(
        {1, 2, 3, 4, 5, 6, 7}, {1, 2, 3, 4, 5, 6, 7}, {}, {}, {});

    groups.push_back({5, {1, 2}});
    groups.push_back({9, {3, 4, 5, 6, 7}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors
   * @when pop early terminations
   * @then partition is correct
   */
  TEST_F(PartitionTestV3, PopsEarlyTerminations) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{3, 4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    const RleBitset terminations{1, 3, 5};
    const ChainEpoch termination_epoch{3};
    EXPECT_OUTCOME_TRUE_1(partition.terminateSectors(
        sectors_arr, termination_epoch, terminations, ssize, quant));

    EXPECT_OUTCOME_TRUE(result1, partition.popEarlyTerminations(1));
    const auto &[termination_res1, has_more1] = result1;
    const RleBitset expected_sectors1{1};
    EXPECT_EQ(termination_res1.sectors.at(termination_epoch),
              expected_sectors1);
    EXPECT_TRUE(has_more1);

    const BitfieldQueue<kEearlyTerminatedBitWidth> queue1{
        partition.early_terminated, kNoQuantization};
    EXPECT_OUTCOME_EQ(queue1.queue.size(), 1);
    EXPECT_OUTCOME_TRUE(terminated1, queue1.queue.get(termination_epoch));
    const RleBitset expected_terminated1{3, 5};
    EXPECT_EQ(terminated1, expected_terminated1);

    EXPECT_OUTCOME_TRUE(result2, partition.popEarlyTerminations(5));
    const auto &[termination_res2, has_more2] = result2;
    const RleBitset expected_sectors2{3, 5};
    EXPECT_EQ(termination_res2.sectors.at(termination_epoch),
              expected_sectors2);
    EXPECT_FALSE(has_more2);

    const BitfieldQueue<kEearlyTerminatedBitWidth> queue2{
        partition.early_terminated, kNoQuantization};
    EXPECT_OUTCOME_EQ(queue2.queue.size(), 0);
  }

  /**
   * @given partition without sectors
   * @when add a lot of sectors
   * @then partition is correct, sectors added
   */
  TEST_F(PartitionTestV3, TestMaxSectors) {
    const auto proof_type = RegisteredSealProof::kStackedDrg32GiBV1_1;
    EXPECT_OUTCOME_TRUE(sector_size, getSectorSize(proof_type));
    EXPECT_OUTCOME_TRUE(partition_sectors,
                        getSealProofWindowPoStPartitionSectors(proof_type));

    std::vector<SectorOnChainInfo> many_sectors;
    RleBitset sector_nos;

    for (size_t i = 0; i < partition_sectors; i++) {
      const uint64_t id = uint64_t(i + 1) << 50;
      sector_nos.insert(id);
      many_sectors.push_back(testSector(i + 1, id, 50, 60, 1000));
    }

    EXPECT_OUTCOME_TRUE(power,
                        partition.addSectors(
                            false, many_sectors, sector_size, kNoQuantization));
    EXPECT_EQ(power, powerForSectors(ssize, many_sectors));

    sectors = many_sectors;
    quant = kNoQuantization;
    assertPartitionState(sector_nos, {}, {}, {}, sector_nos);
  }

  /**
   * @given partition with proven sectors
   * @when record skipped faults are not exist
   * @then return error
   */
  TEST_F(PartitionTestV3, FailIfAllDeclaredSectorsAreNonInPartition) {
    setupProven();
    const auto sectors_arr = getSectorsArray();

    const RleBitset skipped{1, 100};

    EXPECT_OUTCOME_ERROR(
        VMExitCode::kErrIllegalArgument,
        partition.recordSkippedFaults(sectors_arr, ssize, quant, exp, skipped));
  }

  /**
   * @given partition with proven sectors
   * @when record skipped faults intersected with faults and terminations
   * @then partition is correct, only not intersected skipped faults are added
   */
  TEST_F(PartitionTestV3, AlreadyFaultyAndTerminatedSectorsAreIgnored) {
    setupProven();
    const auto sectors_arr = getSectorsArray();

    const RleBitset terminations{1, 2};
    EXPECT_OUTCOME_TRUE_1(
        partition.terminateSectors(sectors_arr, 3, terminations, ssize, quant));
    assertPartitionState({1, 2, 3, 4, 5, 6}, {}, {}, terminations, {});

    const RleBitset fault_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));
    assertPartitionState({1, 2, 3, 4, 5, 6}, fault_set, {}, terminations, {});

    const RleBitset skipped{1, 2, 3, 4, 5};
    EXPECT_OUTCOME_TRUE(
        result,
        partition.recordSkippedFaults(sectors_arr, ssize, quant, exp, skipped));
    const auto &[power_delta, new_fault_power, retracted_power, new_faults] =
        result;

    EXPECT_EQ(retracted_power, PowerPair());
    EXPECT_EQ(new_fault_power,
              powerForSectors(ssize, selectSectorsTest(sectors, {3})));
    EXPECT_EQ(power_delta, new_fault_power.negative());
    EXPECT_TRUE(new_faults);

    assertPartitionState({1, 2, 3, 4, 5, 6}, {3, 4, 5}, {}, {1, 2}, {});
  }

  /**
   * @given partition with proven sectors
   * @when record skipped faults intersected with recoveries
   * @then partition is correct, recoveries retracted
   */
  TEST_F(PartitionTestV3,
         RecoveriesAreRetractedWithoutBeingMarkedAsNewFaultyPower) {
    setupProven();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {4, 5}, {}, {});

    const RleBitset skipped{1, 4, 5};
    EXPECT_OUTCOME_TRUE(
        result,
        partition.recordSkippedFaults(sectors_arr, ssize, quant, exp, skipped));
    const auto &[power_delta, new_fault_power, recovery_power, new_faults] =
        result;

    const auto expected_faulty_power =
        powerForSectors(ssize, selectSectorsTest(sectors, {1}));
    EXPECT_EQ(new_fault_power, expected_faulty_power);
    EXPECT_EQ(power_delta, expected_faulty_power.negative());
    EXPECT_EQ(recovery_power,
              powerForSectors(ssize, selectSectorsTest(sectors, {4, 5})));
    EXPECT_TRUE(new_faults);

    assertPartitionState({1, 2, 3, 4, 5, 6}, {1, 4, 5, 6}, {}, {}, {});
  }

  /**
   * @given partition with proven sectors
   * @when record empty skipped faults
   * @then partition is correct
   */
  TEST_F(PartitionTestV3, SuccessfulWhenSkippedFaultSetIsEmpty) {
    setupProven();
    const auto sectors_arr = getSectorsArray();

    EXPECT_OUTCOME_TRUE(
        result,
        partition.recordSkippedFaults(sectors_arr, ssize, quant, exp, {}));
    const auto &[power_delta, new_fault_power, recovery_power, new_faults] =
        result;

    EXPECT_EQ(power_delta, PowerPair());
    EXPECT_EQ(new_fault_power, PowerPair());
    EXPECT_EQ(recovery_power, PowerPair());
    EXPECT_FALSE(new_faults);

    assertPartitionState({1, 2, 3, 4, 5, 6}, {}, {}, {}, {});
  }

}  // namespace fc::vm::actor::builtin::v3::miner
