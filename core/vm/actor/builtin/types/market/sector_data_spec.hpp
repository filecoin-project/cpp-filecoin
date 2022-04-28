/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::market {
  using primitives::DealId;
  using primitives::sector::RegisteredSealProof;

  struct SectorDataSpec {
    std::vector<DealId> deals;
    RegisteredSealProof sector_type;

    inline bool operator==(const SectorDataSpec &other) const {
      return deals == other.deals && sector_type == other.sector_type;
    }

    inline bool operator!=(const SectorDataSpec &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(SectorDataSpec, deals, sector_type)

}  // namespace fc::vm::actor::builtin::types::market
