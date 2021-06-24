/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/partition_sector_map.hpp"

namespace fc::vm::actor::builtin::types::miner {

  struct DeadlineSectorMap {
    std::map<uint64_t, PartitionSectorMap> map;

    outcome::result<void> add(uint64_t dl_id,
                              uint64_t part_id,
                              const RleBitset &sector_nos);

    outcome::result<std::tuple<uint64_t, uint64_t>> count() const;

    std::vector<uint64_t> deadlines() const;

    outcome::result<void> check(uint64_t max_partitions,
                                uint64_t max_sectors) const;
    
    int length() const;
  };

}  // namespace fc::vm::actor::builtin::types::miner
