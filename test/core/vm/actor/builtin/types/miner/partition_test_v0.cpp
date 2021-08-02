/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/types/partition.hpp"

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "test_utils.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/type_manager/type_manager.hpp"
#include "vm/actor/version.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::sector::getSealProofWindowPoStPartitionSectors;
  using primitives::sector::getSectorSize;
  using primitives::sector::RegisteredSealProof;
  using runtime::MockRuntime;
  using storage::ipfs::InMemoryDatastore;
  using types::TypeManager;
  using types::miner::BitfieldQueue;
  using types::miner::kNoQuantization;
  using types::miner::powerForSectors;
  using types::miner::Sectors;

  struct ExpectExpirationGroup {
    ChainEpoch expiration{};
    RleBitset sectors;
  };

  struct PartitionTestV0 : testing::Test {
    void SetUp() override {
      actor_version = ActorVersion::kVersion0;
      ipld->actor_version = actor_version;
      cbor_blake::cbLoadT(ipld, partition);

      EXPECT_CALL(runtime, getIpfsDatastore())
          .WillRepeatedly(testing::Invoke([&]() { return ipld; }));

      EXPECT_CALL(runtime, getActorVersion())
          .WillRepeatedly(testing::Invoke([&]() { return actor_version; }));
    }

    void setup() {
      sectors = {testSector(2, 1, 50, 60, 1000),
                 testSector(3, 2, 51, 61, 1001),
                 testSector(7, 3, 52, 62, 1002),
                 testSector(8, 4, 53, 63, 1003),
                 testSector(11, 5, 54, 64, 1004),
                 testSector(13, 6, 55, 65, 1005)};

      EXPECT_OUTCOME_TRUE(
          power, partition.addSectors(runtime, false, sectors, ssize, quant));

      EXPECT_EQ(power, powerForSectors(ssize, sectors));
    }

    void assertPartitionExpirationQueue() const {
      EXPECT_OUTCOME_TRUE(queue,
                          TypeManager::loadExpirationQueue(
                              runtime, partition.expirations_epochs, quant));

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

      const auto live_sectors = selectSectors(sectors, live);

      const auto faulty_power =
          powerForSectors(ssize, selectSectors(sectors, partition.faults));
      EXPECT_EQ(faulty_power, partition.faulty_power);

      const auto recovering_power =
          powerForSectors(ssize, selectSectors(sectors, partition.recoveries));
      EXPECT_EQ(recovering_power, partition.recovering_power);

      const auto live_power = powerForSectors(ssize, live_sectors);
      EXPECT_EQ(live_power, partition.live_power);

      const auto active_power = live_power - faulty_power;
      EXPECT_EQ(active_power, partition.activePower());

      EXPECT_TRUE(partition.faults.contains(partition.recoveries));
      EXPECT_TRUE(live.contains(partition.faults));
      EXPECT_TRUE(partition.sectors.contains(partition.terminated));
      EXPECT_FALSE(live.containsAny(partition.terminated));
      EXPECT_TRUE(live.contains(active));
      EXPECT_FALSE(active.containsAny(partition.faults));

      {
        std::set<SectorNumber> seen_sectors;

        EXPECT_OUTCOME_TRUE(exp_q,
                            TypeManager::loadExpirationQueue(
                                runtime, partition.expirations_epochs, quant));

        auto visitor{[&](ChainEpoch epoch,
                         const ExpirationSet &es) -> outcome::result<void> {
          EXPECT_EQ(quant.quantizeUp(epoch), epoch);

          const auto all = es.on_time_sectors + es.early_sectors;
          const auto active = all - partition.faults;
          const auto faulty = all.intersect(partition.faults);

          const auto active_sectors = selectSectors(live_sectors, active);
          const auto faulty_sectors = selectSectors(live_sectors, faulty);
          const auto on_time_sectors =
              selectSectors(live_sectors, es.on_time_sectors);
          const auto early_sectors =
              selectSectors(live_sectors, es.early_sectors);

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
        const BitfieldQueue early_q{partition.early_terminated,
                                    kNoQuantization};
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
                              const RleBitset &terminations) const {
      EXPECT_EQ(partition.faults, faults);
      EXPECT_EQ(partition.recoveries, recovering);
      EXPECT_EQ(partition.terminated, terminations);
      EXPECT_EQ(partition.sectors, all_sectors_ids);

      checkPartitionInvariants();
    }

    Sectors getSectorsArray() const {
      Sectors sectors_arr;
      cbor_blake::cbLoadT(ipld, sectors_arr);

      EXPECT_OUTCOME_TRUE_1(sectors_arr.store(sectors));
      return sectors_arr;
    }

    std::vector<SectorOnChainInfo> selectSectors(
        const std::vector<SectorOnChainInfo> &source_sectors,
        const RleBitset &field) const {
      auto to_include = field;
      std::vector<SectorOnChainInfo> included;

      for (const auto &sector : source_sectors) {
        if (!to_include.has(sector.sector)) {
          continue;
        }
        included.push_back(sector);
        to_include.erase(sector.sector);
      }
      EXPECT_TRUE(to_include.empty());
      return included;
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

    Partition partition;
    std::vector<ExpectExpirationGroup> groups;
  };

  /**
   * @given partition with sectors
   * @when check partition state
   * @then partition is correct
   */
  TEST_F(PartitionTestV0, AddsSectorsAndReportsSectorStats) {
    setup();

    assertPartitionState({1, 2, 3, 4, 5, 6}, {}, {}, {});

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
  TEST_F(PartitionTestV0, DoesntAddSectorsTwice) {
    setup();

    const auto result = partition.addSectors(
        runtime, false, slice(sectors, 0, 1), ssize, quant);
    EXPECT_EQ(result.error().message(), "not all added sectors are new");
  }

  /**
   * @given partition with sectors
   * @when add some correct faults
   * @then partition is correct
   */
  TEST_F(PartitionTestV0, AddsFaults) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5};
    PowerPair power;
    EXPECT_OUTCOME_TRUE(result,
                        partition.recordFaults(
                            runtime, sectors_arr, fault_set, 7, ssize, quant));
    std::tie(std::ignore, std::ignore, power) = result;

    EXPECT_EQ(power, powerForSectors(ssize, selectSectors(sectors, fault_set)));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5}, {}, {});

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
  TEST_F(PartitionTestV0, ReAddingFaultsIsANoOp) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set1{4, 5};
    PowerPair power1;
    EXPECT_OUTCOME_TRUE(result1,
                        partition.recordFaults(
                            runtime, sectors_arr, fault_set1, 7, ssize, quant));
    std::tie(std::ignore, std::ignore, power1) = result1;
    EXPECT_EQ(power1,
              powerForSectors(ssize, selectSectors(sectors, fault_set1)));

    const RleBitset fault_set2{5, 6};
    PowerPair power2;
    RleBitset new_faults;
    EXPECT_OUTCOME_TRUE(result2,
                        partition.recordFaults(
                            runtime, sectors_arr, fault_set2, 3, ssize, quant));
    std::tie(new_faults, std::ignore, power2) = result2;
    const RleBitset expected_new_faults{6};
    EXPECT_EQ(new_faults, expected_new_faults);
    EXPECT_EQ(power2, powerForSectors(ssize, selectSectors(sectors, {6})));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {}, {});

    groups.push_back({5, {1, 2, 6}});
    groups.push_back({9, {3, 4, 5}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors
   * @when add fault for missing sector
   * @then return error
   */
  TEST_F(PartitionTestV0, FailsToAddFaultsForMissingSectors) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{99};
    const auto result = partition.recordFaults(
        runtime, sectors_arr, fault_set, 7, ssize, quant);
    EXPECT_EQ(result.error().message(), "failed fault declaration");
  }

  /**
   * @given partition with sectors
   * @when add some correct recoveries for faults
   * @then partition is correct
   */
  TEST_F(PartitionTestV0, AddsRecoveries) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(partition.recordFaults(
        runtime, sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {4, 5}, {});
  }

  /**
   * @given partition with sectors
   * @when declare some faults to recover and then add these faults again
   * @then partition is correct, recoveries don't contain faults added again
   */
  TEST_F(PartitionTestV0, RemoveRecoveries) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(partition.recordFaults(
        runtime, sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    RleBitset new_faults1;
    EXPECT_OUTCOME_TRUE(
        result1,
        partition.recordFaults(runtime, sectors_arr, {}, 7, ssize, quant));
    std::tie(new_faults1, std::ignore, std::ignore) = result1;
    EXPECT_TRUE(new_faults1.empty());

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {4, 5}, {});

    RleBitset new_faults2;
    EXPECT_OUTCOME_TRUE(
        result2,
        partition.recordFaults(runtime, sectors_arr, {5}, 10, ssize, quant));
    std::tie(new_faults2, std::ignore, std::ignore) = result2;
    EXPECT_TRUE(new_faults2.empty());

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {4}, {});
  }

  /**
   * @given partition with sectors and faults
   * @when recover faults
   * @then partition is correct and doesn't contain these faults anymore
   */
  TEST_F(PartitionTestV0, RecoversFaults) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(partition.recordFaults(
        runtime, sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    EXPECT_OUTCOME_TRUE(
        recovered_power,
        partition.recoverFaults(runtime, sectors_arr, ssize, quant));
    EXPECT_EQ(recovered_power,
              powerForSectors(ssize, selectSectors(sectors, recover_set)));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {6}, {}, {});

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
  TEST_F(PartitionTestV0, FaultyPowerRecoveredExactlyOnce) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(partition.recordFaults(
        runtime, sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{3, 4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, fault_set));

    EXPECT_EQ(partition.recovering_power,
              powerForSectors(ssize, selectSectors(sectors, fault_set)));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 5, 6}, {4, 5, 6}, {});
  }

  /**
   * @given partition with sectors
   * @when recover non-existed fault
   * @then return error
   */
  TEST_F(PartitionTestV0, MissingSectorsAreNotRecovered) {
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
  TEST_F(PartitionTestV0, ReschedulesExpirations) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{2};
    EXPECT_OUTCOME_TRUE_1(partition.recordFaults(
        runtime, sectors_arr, fault_set, 7, ssize, quant));

    EXPECT_OUTCOME_TRUE(moved,
                        partition.rescheduleExpirationsV0(
                            runtime, sectors_arr, 18, {2, 4, 6}, ssize, quant));
    const RleBitset expected_moved{4, 6};
    EXPECT_EQ(moved, expected_moved);

    sectors = rescheduleSectors(18, {4, 6});

    assertPartitionState({1, 2, 3, 4, 5, 6}, {2}, {}, {});

    groups.push_back({5, {1, 2}});
    groups.push_back({9, {3}});
    groups.push_back({13, {5}});
    groups.push_back({21, {4, 6}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors
   * @when replace some sectors
   * @then partition is correct, sectors are replaced
   */
  TEST_F(PartitionTestV0, ReplaceSectors) {
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

    EXPECT_OUTCOME_TRUE(result,
                        partition.replaceSectors(
                            runtime, old_sectors, new_sectors, ssize, quant));
    const auto &[power_delta, pledge_delta] = result;

    EXPECT_EQ(power_delta, new_sector_power - old_sector_power);
    EXPECT_EQ(pledge_delta, new_sector_pledge - old_sector_pledge);

    auto all_sectors = new_sectors;
    all_sectors.insert(all_sectors.end(), sectors.begin(), sectors.begin() + 1);
    all_sectors.insert(all_sectors.end(), sectors.begin() + 4, sectors.end());

    sectors = all_sectors;

    assertPartitionState({1, 2, 5, 6, 7, 8}, {}, {}, {});

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
  TEST_F(PartitionTestV0,
         ReplaceSectorsErrorsWhenAttemptingToReplaceInactiveSector) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{2};
    EXPECT_OUTCOME_TRUE_1(partition.recordFaults(
        runtime, sectors_arr, fault_set, 7, ssize, quant));

    const auto old_sectors = slice(sectors, 1, 4);
    const std::vector<SectorOnChainInfo> new_sectors = {
        testSector(10, 2, 150, 260, 3000)};

    const auto result = partition.replaceSectors(
        runtime, old_sectors, new_sectors, ssize, quant);
    EXPECT_EQ(result.error().message(), "refusing to replace inactive sectors");
  }

  /**
   * @given partition with sectors
   * @when terminate some sectors (include faults and recoveries)
   * @then partition is correct, sectors are terminated
   */
  TEST_F(PartitionTestV0, TerminateSectors) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{3, 4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(partition.recordFaults(
        runtime, sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    const RleBitset terminations{1, 3, 5};
    const ChainEpoch termination_epoch{3};
    EXPECT_OUTCOME_TRUE(removed,
                        partition.terminateSectors(runtime,
                                                   sectors_arr,
                                                   termination_epoch,
                                                   terminations,
                                                   ssize,
                                                   quant));

    EXPECT_EQ(removed.active_power,
              powerForSectors(ssize, selectSectors(sectors, {1})));
    EXPECT_EQ(removed.faulty_power,
              powerForSectors(ssize, selectSectors(sectors, {3, 5})));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {4, 6}, {4}, terminations);

    groups.push_back({5, {2}});
    groups.push_back({9, {4, 6}});
    assertPartitionExpirationQueue();

    const BitfieldQueue queue{partition.early_terminated, kNoQuantization};
    EXPECT_OUTCOME_EQ(queue.queue.size(), 1);
    EXPECT_OUTCOME_TRUE(terminated, queue.queue.get(termination_epoch));
    EXPECT_EQ(terminated, terminations);
  }

  /**
   * @given partition with sectors
   * @when terminate non-existed sector
   * @then return error
   */
  TEST_F(PartitionTestV0, TerminateNonExistentSectors) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const auto result =
        partition.terminateSectors(runtime, sectors_arr, 3, {99}, ssize, quant);
    EXPECT_EQ(result.error().message(), "can only terminate live sectors");
  }

  /**
   * @given partition with sectors
   * @when terminate already terminated sectors
   * @then return error
   */
  TEST_F(PartitionTestV0, TerminateAlreadyTerminatedSector) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset terminations{1};
    const ChainEpoch termination_epoch{3};
    EXPECT_OUTCOME_TRUE(removed,
                        partition.terminateSectors(runtime,
                                                   sectors_arr,
                                                   termination_epoch,
                                                   terminations,
                                                   ssize,
                                                   quant));
    EXPECT_EQ(removed.active_power,
              powerForSectors(ssize, selectSectors(sectors, {1})));
    EXPECT_EQ(removed.faulty_power, PowerPair());
    EXPECT_EQ(removed.count(), 1);

    const auto result = partition.terminateSectors(
        runtime, sectors_arr, termination_epoch, terminations, ssize, quant);
    EXPECT_EQ(result.error().message(), "can only terminate live sectors");
  }

  /**
   * @given partition with sectors
   * @when add faults sectors are already terminated
   * @then partition is correct, sectors are moved to faults from termination
   */
  TEST_F(PartitionTestV0, MarkTerminatedSectorsAsFaulty) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset terminations{1};
    const ChainEpoch termination_epoch{3};
    EXPECT_OUTCOME_TRUE_1(partition.terminateSectors(
        runtime, sectors_arr, termination_epoch, terminations, ssize, quant));

    RleBitset new_faults;
    EXPECT_OUTCOME_TRUE(
        result,
        partition.recordFaults(
            runtime, sectors_arr, terminations, 5, ssize, quant));
    std::tie(new_faults, std::ignore, std::ignore) = result;
    EXPECT_TRUE(new_faults.empty());
  }

  /**
   * @given partition with sectors
   * @when pop expired sectors
   * @then partition is correct
   */
  TEST_F(PartitionTestV0, PopExpiringSectors) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4};
    EXPECT_OUTCOME_TRUE_1(partition.recordFaults(
        runtime, sectors_arr, fault_set, 2, ssize, quant));

    const ChainEpoch expire_epoch{5};
    EXPECT_OUTCOME_TRUE(
        exp_set, partition.popExpiredSectors(runtime, expire_epoch, quant));

    const RleBitset expected_on_time_sectors{1, 2};
    EXPECT_EQ(exp_set.on_time_sectors, expected_on_time_sectors);

    const RleBitset expected_early_sectors{4};
    EXPECT_EQ(exp_set.early_sectors, expected_early_sectors);

    EXPECT_EQ(exp_set.on_time_pledge, 1000 + 1001);

    EXPECT_EQ(exp_set.active_power,
              powerForSectors(ssize, slice(sectors, 0, 2)));

    EXPECT_EQ(exp_set.faulty_power,
              powerForSectors(ssize, slice(sectors, 3, 4)));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {}, {}, {1, 2, 4});

    groups.push_back({9, {3}});
    groups.push_back({13, {5, 6}});
    assertPartitionExpirationQueue();

    const BitfieldQueue queue{partition.early_terminated, kNoQuantization};
    EXPECT_OUTCOME_EQ(queue.queue.size(), 1);
    EXPECT_OUTCOME_TRUE(expired, queue.queue.get(expire_epoch));
    EXPECT_EQ(expired, fault_set);
  }

  /**
   * @given partition with sectors
   * @when pop recovered sector
   * @then return error
   */
  TEST_F(PartitionTestV0, PopExpiringSectorsErrorsIfARecoveryExists) {
    setup();
    const auto sectors_arr = getSectorsArray();

    EXPECT_OUTCOME_TRUE_1(
        partition.recordFaults(runtime, sectors_arr, {5}, 2, ssize, quant));
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, {5}));

    const auto result = partition.popExpiredSectors(runtime, 5, quant);
    EXPECT_EQ(result.error().message(),
              "unexpected recoveries while processing expirations");
  }

  /**
   * @given partition with sectors
   * @when record missing post
   * @then partition is correct
   */
  TEST_F(PartitionTestV0, RecordsMissingPost) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(partition.recordFaults(
        runtime, sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    EXPECT_OUTCOME_TRUE(result,
                        partition.recordMissedPostV0(runtime, 6, quant));
    const auto &[new_fault_power, failed_recovery_power] = result;

    EXPECT_EQ(new_fault_power, powerForSectors(ssize, slice(sectors, 0, 3)));
    EXPECT_EQ(failed_recovery_power,
              powerForSectors(ssize, slice(sectors, 3, 5)));

    assertPartitionState({1, 2, 3, 4, 5, 6}, {1, 2, 3, 4, 5, 6}, {}, {});

    groups.push_back({5, {1, 2}});
    groups.push_back({9, {3, 4, 5, 6}});
    assertPartitionExpirationQueue();
  }

  /**
   * @given partition with sectors
   * @when pop early terminations
   * @then partition is correct
   */
  TEST_F(PartitionTestV0, PopsEarlyTerminations) {
    setup();
    const auto sectors_arr = getSectorsArray();

    const RleBitset fault_set{3, 4, 5, 6};
    EXPECT_OUTCOME_TRUE_1(partition.recordFaults(
        runtime, sectors_arr, fault_set, 7, ssize, quant));

    const RleBitset recover_set{4, 5};
    EXPECT_OUTCOME_TRUE_1(
        partition.declareFaultsRecovered(sectors_arr, ssize, recover_set));

    const RleBitset terminations{1, 3, 5};
    const ChainEpoch termination_epoch{3};
    EXPECT_OUTCOME_TRUE_1(partition.terminateSectors(
        runtime, sectors_arr, termination_epoch, terminations, ssize, quant));

    EXPECT_OUTCOME_TRUE(result1, partition.popEarlyTerminations(runtime, 1));
    const auto &[termination_res1, has_more1] = result1;
    const RleBitset expected_sectors1{1};
    EXPECT_EQ(termination_res1.sectors.at(termination_epoch),
              expected_sectors1);
    EXPECT_TRUE(has_more1);

    const BitfieldQueue queue1{partition.early_terminated, kNoQuantization};
    EXPECT_OUTCOME_EQ(queue1.queue.size(), 1);
    EXPECT_OUTCOME_TRUE(terminated1, queue1.queue.get(termination_epoch));
    const RleBitset expected_terminated1{3, 5};
    EXPECT_EQ(terminated1, expected_terminated1);

    EXPECT_OUTCOME_TRUE(result2, partition.popEarlyTerminations(runtime, 5));
    const auto &[termination_res2, has_more2] = result2;
    const RleBitset expected_sectors2{3, 5};
    EXPECT_EQ(termination_res2.sectors.at(termination_epoch),
              expected_sectors2);
    EXPECT_FALSE(has_more2);

    const BitfieldQueue queue2{partition.early_terminated, kNoQuantization};
    EXPECT_OUTCOME_EQ(queue2.queue.size(), 0);
  }

  /**
   * @given partition without sectors
   * @when add a lot of sectors
   * @then partition is correct, sectors added
   */
  TEST_F(PartitionTestV0, TestMaxSectors) {
    const auto proof_type = RegisteredSealProof::kStackedDrg32GiBV1;
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

    EXPECT_OUTCOME_TRUE(
        power,
        partition.addSectors(
            runtime, false, many_sectors, sector_size, kNoQuantization));
    EXPECT_EQ(power, powerForSectors(sector_size, many_sectors));

    sectors = many_sectors;
    quant = kNoQuantization;
    assertPartitionState(sector_nos, {}, {}, {});
  }

}  // namespace fc::vm::actor::builtin::v0::miner
