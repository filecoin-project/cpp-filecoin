/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/cid/cid.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::DealId;
  using primitives::SectorNumber;
  using primitives::sector::RegisteredUpdateProof;

  struct ReplicaUpdate {
    SectorNumber sector{};
    uint64_t deadline{};
    uint64_t partition{};
    CID new_sealed_sector_cid;
    std::vector<DealId> deals;
    RegisteredUpdateProof update_type{};
    Bytes proof;

    inline bool operator==(const ReplicaUpdate &other) const {
      return sector == other.sector && deadline == other.deadline
             && partition == other.partition
             && new_sealed_sector_cid == other.new_sealed_sector_cid
             && deals == other.deals && update_type == other.update_type
             && proof == other.proof;
    }

    inline bool operator!=(const ReplicaUpdate &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(ReplicaUpdate,
             sector,
             deadline,
             partition,
             new_sealed_sector_cid,
             deals,
             update_type,
             proof)

}  // namespace fc::vm::actor::builtin::types::miner
