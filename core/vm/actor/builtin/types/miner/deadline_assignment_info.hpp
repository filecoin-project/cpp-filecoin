/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cassert>
#include <cstdint>

namespace fc::vm::actor::builtin::types::miner {

  struct DeadlineAssignmentInfo {
    size_t index{};
    uint64_t live_sectors{};
    uint64_t total_sectors{};

    inline uint64_t partitionsAfterAssignment(uint64_t partition_size) const {
      assert(partition_size != 0);
      const auto sector_count = total_sectors + 1;
      const uint64_t full_partitions = sector_count / partition_size;
      return (sector_count % partition_size == 0) ? full_partitions
                                                  : full_partitions + 1;
    }

    inline uint64_t compactPartitionsAfterAssignment(
        uint64_t partition_size) const {
      assert(partition_size != 0);
      const auto sector_count = live_sectors + 1;
      const uint64_t full_partitions = sector_count / partition_size;
      return (sector_count % partition_size == 0) ? full_partitions
                                                  : full_partitions + 1;
    }

    inline bool isFullNow(uint64_t partition_size) const {
      assert(partition_size != 0);
      return (total_sectors % partition_size == 0);
    }

    inline bool maxPartitionsReached(uint64_t partition_size,
                                     uint64_t max_partitions) const {
      return total_sectors >= partition_size * max_partitions;
    }
  };

}  // namespace fc::vm::actor::builtin::types::miner
