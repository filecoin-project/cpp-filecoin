/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadline_sector_map.hpp"

#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {

  outcome::result<void> DeadlineSectorMap::add(uint64_t dl_id,
                                               uint64_t part_id,
                                               const RleBitset &sector_nos) {
    if (dl_id >= kWPoStPeriodDeadlines) {
      return ERROR_TEXT("invalid deadline");
    }

    map[dl_id].add(part_id, sector_nos);

    return outcome::success();
  }

  outcome::result<std::tuple<uint64_t, uint64_t>> DeadlineSectorMap::count()
      const {
    uint64_t partitions{0};
    uint64_t sectors{0};

    for (const auto &[dl_id, pm] : map) {
      OUTCOME_TRY(result, pm.count());
      const auto &[part_count, sector_count] = result;

      if (part_count > UINT64_MAX - partitions) {
        return ERROR_TEXT("uint64 overflow when counting partitions");
      }

      if (sector_count > UINT64_MAX - sectors) {
        return ERROR_TEXT("uint64 overflow when counting sectors");
      }

      partitions += part_count;
      sectors += sector_count;
    }

    return std::make_tuple(partitions, sectors);
  }

  std::vector<uint64_t> DeadlineSectorMap::deadlines() const {
    std::vector<uint64_t> deadlines;

    for (const auto &[dl_id, pm] : map) {
      deadlines.push_back(dl_id);
    }

    return deadlines;
  }

  outcome::result<void> DeadlineSectorMap::check(uint64_t max_partitions,
                                                 uint64_t max_sectors) const {
    OUTCOME_TRY(result, count());
    const auto &[partition_count, sector_count] = result;

    if (partition_count > max_partitions) {
      return ERROR_TEXT("too many partitions");
    }

    if (sector_count > max_sectors) {
      return ERROR_TEXT("too many sectors");
    }

    return outcome::success();
  }

  int DeadlineSectorMap::length() const {
    return map.size();
  }

}  // namespace fc::vm::actor::builtin::types::miner
