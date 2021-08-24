/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/partition_sector_map.hpp"
#include "vm/actor/builtin/types/miner/types.hpp"

namespace fc::vm::actor::builtin::types::miner {

  struct DisputeInfo {
    RleBitset all_sector_nos;
    RleBitset ignored_sector_nos;
    PartitionSectorMap disputed_sectors;
    PowerPair disputed_power;
  };

}  // namespace fc::vm::actor::builtin::types::miner
