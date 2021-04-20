/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;

  struct TerminationResult {
    std::map<ChainEpoch, RleBitset> sectors;
    uint64_t partitions_processed{};
    uint64_t sectors_processed{};

    /**
     * Merge RleBitsets of sectors with one epoch and add new result's values to
     * current result.
     * @param new_result - new result to be added
     */
    void add(const TerminationResult &new_result);
    bool belowLimit(uint64_t max_partitions, uint64_t max_sectors) const;
    bool isEmpty() const;
  };

}  // namespace fc::vm::actor::builtin::types::miner
