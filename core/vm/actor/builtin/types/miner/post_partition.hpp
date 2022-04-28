/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::RleBitset;

  struct PoStPartition {
    /// Partitions are numbered per-deadline, from zero
    uint64_t index{0};
    // Sectors skipped while proving that weren't already declared faulty
    RleBitset skipped;

    inline bool operator==(const PoStPartition &other) const {
      return index == other.index && skipped == other.skipped;
    }

    inline bool operator!=(const PoStPartition &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(PoStPartition, index, skipped)

}  // namespace fc::vm::actor::builtin::types::miner
