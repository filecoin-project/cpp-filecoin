/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/termination.hpp"

namespace fc::vm::actor::builtin::types::miner {

  void TerminationResult::add(const TerminationResult &new_result) {
    partitions_processed += new_result.partitions_processed;
    sectors_processed += new_result.sectors_processed;
    for (const auto &[epoch, new_sectors] : new_result.sectors) {
      sectors[epoch] += new_sectors;
    }
  }

  bool TerminationResult::belowLimit(uint64_t max_partitions,
                                     uint64_t max_sectors) const {
    return (partitions_processed < max_partitions)
           && (sectors_processed < max_sectors);
  }

  bool TerminationResult::isEmpty() const {
    return sectors_processed == 0;
  }

}  // namespace fc::vm::actor::builtin::types::miner
