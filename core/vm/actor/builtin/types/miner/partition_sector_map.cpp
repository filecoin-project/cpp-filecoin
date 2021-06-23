/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/partition_sector_map.hpp"

#include "common/error_text.hpp"

namespace fc::vm::actor::builtin::types::miner {

  void PartitionSectorMap::add(uint64_t part_id, const RleBitset &sector_nos) {
    m[part_id] += sector_nos;
  }

  outcome::result<std::tuple<uint64_t, uint64_t>> PartitionSectorMap::count()
      const {
    uint64_t sectors{0};

    for (const auto &[part_id, bf] : m) {
      const auto count = bf.size();
      if (count > UINT64_MAX - sectors) {
        return ERROR_TEXT("uint64 overflow when counting sectors");
      }
      sectors += count;
    }

    return std::make_tuple(m.size(), sectors);
  }

  std::vector<uint64_t> PartitionSectorMap::partitions() const {
    std::vector<uint64_t> partitions;

    for (const auto &[part_id, bf] : m) {
      partitions.push_back(part_id);
    }

    return partitions;
  }

  int PartitionSectorMap::length() const {
    return m.size();
  }

}  // namespace fc::vm::actor::builtin::types::miner
