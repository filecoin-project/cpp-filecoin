/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline_assignment_heap.hpp"

#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {

  void DeadlineAssignmentHeap::swap(int i, int j) {
    std::swap(deadline_infos[i], deadline_infos[j]);
  }

  bool DeadlineAssignmentHeap::less(int i, int j) const {
    const auto &a = deadline_infos[i];
    const auto &b = deadline_infos[j];

    const bool a_max_partition_reached =
        a.maxPartitionsReached(partition_size, max_partitions);
    const bool b_max_partition_reached =
        b.maxPartitionsReached(partition_size, max_partitions);
    if (a_max_partition_reached != b_max_partition_reached) {
      return !a_max_partition_reached;
    }

    const auto a_compact_partitions_after_assignment =
        a.compactPartitionsAfterAssignment(partition_size);
    const auto b_compact_partitions_after_assignment =
        b.compactPartitionsAfterAssignment(partition_size);
    if (a_compact_partitions_after_assignment
        != b_compact_partitions_after_assignment) {
      return a_compact_partitions_after_assignment
             < b_compact_partitions_after_assignment;
    }

    const auto a_partitions_after_assignment =
        a.partitionsAfterAssignment(partition_size);
    const auto b_partitions_after_assignment =
        b.partitionsAfterAssignment(partition_size);
    if (a_partitions_after_assignment != b_partitions_after_assignment) {
      return a_partitions_after_assignment < b_partitions_after_assignment;
    }

    const auto a_is_full_now = a.isFullNow(partition_size);
    const auto b_is_full_now = b.isFullNow(partition_size);
    if (a_is_full_now != b_is_full_now) {
      return !a_is_full_now;
    }

    if (!a_is_full_now && !b_is_full_now) {
      if (a.total_sectors != b.total_sectors) {
        return a.total_sectors > b.total_sectors;
      }
    }

    if (a.live_sectors != b.live_sectors) {
      return a.live_sectors < b.live_sectors;
    }

    return a.index < b.index;
  }

  DeadlineAssignmentInfo DeadlineAssignmentHeap::pop() {
    const auto last = deadline_infos.back();
    deadline_infos.pop_back();
    return last;
  }

  outcome::result<std::vector<std::vector<SectorOnChainInfo>>> assignDeadlines(
      uint64_t max_partitions,
      uint64_t partition_size,
      const std::vector<Universal<Deadline>> &deadlines,
      const std::vector<SectorOnChainInfo> &sectors) {
    DeadlineAssignmentHeap dl_heap{.max_partitions = max_partitions,
                                   .partition_size = partition_size,
                                   .deadline_infos = {}};

    for (size_t dl_id = 0; dl_id < deadlines.size(); dl_id++) {
      dl_heap.deadline_infos.push_back(DeadlineAssignmentInfo{
          .index = static_cast<int>(dl_id),
          .live_sectors = deadlines[dl_id]->live_sectors,
          .total_sectors = deadlines[dl_id]->total_sectors});
    }

    // todo heap.init

    std::vector<std::vector<SectorOnChainInfo>> changes(
        kWPoStPeriodDeadlines, std::vector<SectorOnChainInfo>{});

    for (const auto &sector : sectors) {
      auto &info = dl_heap.deadline_infos[0];

      if (info.maxPartitionsReached(partition_size, max_partitions)) {
        return ERROR_TEXT("max partitions limit reached for all deadlines");
      }

      changes[info.index].push_back(sector);
      info.live_sectors++;
      info.total_sectors++;

      // todo heap.fix
    }

    return changes;
  }

}  // namespace fc::vm::actor::builtin::types::miner
