/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline_assignment_heap.hpp"

#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {
  bool DeadlineAssignmentLess::operator()(
      const DeadlineAssignmentInfo &a, const DeadlineAssignmentInfo &b) const {
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

  outcome::result<std::vector<std::vector<size_t>>> assignDeadlines(
      uint64_t max_partitions,
      uint64_t partition_size,
      std::vector<DeadlineAssignmentInfo> deadlines,
      size_t sectors) {
    const DeadlineAssignmentLess less{max_partitions, partition_size};
    const auto cmp{[less](auto &l, auto &r) { return !less(l, r); }};
    std::make_heap(deadlines.begin(), deadlines.end(), cmp);

    std::vector<std::vector<size_t>> changes;
    changes.resize(kWPoStPeriodDeadlines);

    for (size_t i = 0; i < sectors; ++i) {
      std::pop_heap(deadlines.begin(), deadlines.end(), cmp);
      auto &info{deadlines.back()};

      if (info.maxPartitionsReached(partition_size, max_partitions)) {
        return ERROR_TEXT("max partitions limit reached for all deadlines");
      }

      changes[info.index].push_back(i);
      info.live_sectors++;
      info.total_sectors++;

      std::push_heap(deadlines.begin(), deadlines.end(), cmp);
    }

    return changes;
  }

}  // namespace fc::vm::actor::builtin::types::miner
