/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "vm/actor/builtin/types/miner/deadline.hpp"
#include "vm/actor/builtin/types/miner/deadline_assignment_info.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"

namespace fc::vm::actor::builtin::types::miner {

  struct DeadlineAssignmentHeap {
    uint64_t max_partitions{};
    uint64_t partition_size{};
    std::vector<DeadlineAssignmentInfo> deadline_infos;

    void swap(int i, int j);
    bool less(int i, int j) const;
    DeadlineAssignmentInfo pop();
  };

  /**
   * Assigns partitions to deadlines, first filling partial partitions, then
   * adding new partitions to deadlines with the fewest live sectors.
   * @param max_partitions - max value of partitions. NOTE: must be 0 for v0
   */
  outcome::result<std::vector<std::vector<SectorOnChainInfo>>> assignDeadlines(
      uint64_t max_partitions,
      uint64_t partition_size,
      const std::vector<Universal<Deadline>> &deadlines,
      const std::vector<SectorOnChainInfo> &sectors);

}  // namespace fc::vm::actor::builtin::types::miner
