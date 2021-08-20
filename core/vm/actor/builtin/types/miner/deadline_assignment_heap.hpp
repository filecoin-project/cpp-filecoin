/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/go/heap.hpp"
#include "vm/actor/builtin/types/miner/deadline.hpp"
#include "vm/actor/builtin/types/miner/deadline_assignment_info.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::go::IHeap;

  struct DeadlineAssignmentHeap : public IHeap<DeadlineAssignmentInfo> {
    uint64_t max_partitions{};
    uint64_t partition_size{};
    std::vector<DeadlineAssignmentInfo> deadline_infos;

    int length() const override;
    bool less(int i, int j) const override;
    void swap(int i, int j) override;
    void push(const DeadlineAssignmentInfo &element) override;
    DeadlineAssignmentInfo pop() override;
  };

  /**
   * Assigns partitions to deadlines, first filling partial partitions, then
   * adding new partitions to deadlines with the fewest live sectors.
   * @param max_partitions - max value of partitions. NOTE: must be 0 for v0
   * @param deadlines - assignable subset of deadlines
   */
  outcome::result<std::vector<std::vector<size_t>>> assignDeadlines(
      uint64_t max_partitions,
      uint64_t partition_size,
      const std::vector<DeadlineAssignmentInfo> &deadlines,
      size_t sectors);

}  // namespace fc::vm::actor::builtin::types::miner
