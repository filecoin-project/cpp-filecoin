/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"

namespace fc::vm::actor::builtin::v5::market {
  using primitives::DealId;
  using primitives::sector::RegisteredSealProof;

  struct SectorDataSpec {
    std::vector<DealId> deals;
    RegisteredSealProof sector_type;
  };
  CBOR_TUPLE(SectorDataSpec, deals, sector_type)

  struct ComputeDataCommitmentParams {
    std::vector<SectorDataSpec> inputs;
  };
  CBOR_TUPLE(ComputeDataCommitmentParams, inputs)

  struct ComputeDataCommitmentReturn {
    std::vector<CID> commds;
  };
  CBOR_TUPLE(ComputeDataCommitmentReturn, commds)

}  // namespace fc::vm::actor::builtin::v5::market
