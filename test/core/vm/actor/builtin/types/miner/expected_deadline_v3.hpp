/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_utils.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/actor/builtin/types/miner/deadline.hpp"
#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "vm/actor/builtin/types/miner/partition.hpp"
#include "vm/actor/builtin/types/miner/quantize.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::SectorSize;
  using runtime::MockRuntime;
  using types::Universal;
  using types::miner::BitfieldQueue;
  using types::miner::Deadline;
  using types::miner::ExpirationSet;
  using types::miner::kEearlyTerminatedBitWidth;
  using types::miner::kNoQuantization;
  using types::miner::Partition;
  using types::miner::powerForSectors;
  using types::miner::PowerPair;
  using types::miner::QuantSpec;
  using types::miner::SectorOnChainInfo;

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

    void assertDeadline(MockRuntime &runtime,
                        const Universal<Deadline> &deadline) const {
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
      EXPECT_EQ(posts, deadline->partitions_posted);
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
                            const Universal<Deadline> &deadline) const {
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

      EXPECT_OUTCOME_TRUE_1(deadline->partitions.visit(partitions_visitor));

      auto partitions_snapshot_visitor{
          [&](int64_t part_id, const auto &partition) -> outcome::result<void> {
            EXPECT_TRUE(partition->recovering_power.isZero());
            EXPECT_TRUE(partition->recoveries.empty());
            EXPECT_TRUE(partition->unproven_power.isZero());
            EXPECT_TRUE(partition->unproven.empty());

            return outcome::success();
          }};

      EXPECT_OUTCOME_TRUE_1(
          deadline->partitions_snapshot.visit(partitions_snapshot_visitor));

      auto proofs_snapshot_visitor{
          [&](int64_t part_id, const auto &proof) -> outcome::result<void> {
            for (const auto &i : proof.partitions) {
              EXPECT_OUTCOME_TRUE(maybe_partition,
                                  deadline->partitions_snapshot.tryGet(i));
              EXPECT_TRUE(maybe_partition.has_value());
            }

            return outcome::success();
          }};

      EXPECT_OUTCOME_TRUE_1(
          deadline->optimistic_post_submissions_snapshot.visit(
              proofs_snapshot_visitor));

      EXPECT_EQ(deadline->live_sectors,
                all_sectors.size() - all_terminations.size());
      EXPECT_EQ(deadline->total_sectors, all_sectors.size());
      EXPECT_EQ(deadline->faulty_power, all_faulty_power);

      {
        for (const auto &[epoch, partitions] : expected_deadline_exp_queue) {
          EXPECT_OUTCOME_TRUE(bf, deadline->expirations_epochs.get(epoch));
          for (const auto &partition : partitions) {
            EXPECT_TRUE(bf.has(partition));
          }
        }
      }

      EXPECT_EQ(deadline->early_terminations,
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

        auto exp_q = loadExpirationQueue(partition->expirations_epochs, quant);

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

}  // namespace fc::vm::actor::builtin::v3::miner
