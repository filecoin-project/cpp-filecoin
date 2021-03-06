/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::RleBitset;

  struct PartitionSectorMap {
    std::map<uint64_t, RleBitset> map;

    void add(uint64_t part_id, const RleBitset &sector_nos);

    outcome::result<std::tuple<uint64_t, uint64_t>> count() const;

    std::vector<uint64_t> partitions() const;

    int length() const;
  };

}  // namespace fc::vm::actor::builtin::types::miner
