/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/sector/sector.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"

namespace fc::vm::actor::builtin::v5::market {
  using primitives::DealId;
  using primitives::sector::RegisteredSealProof;

  struct SectorDataSpec {
    std::vector<DealId> deals;
    RegisteredSealProof sector_type;
  };
  CBOR_TUPLE(SectorDataSpec, deals, sector_type)

  struct ComputeDataCommitment : ActorMethodBase<8> {
    struct Params {
      std::vector<SectorDataSpec> inputs;
    };
    struct Result {
      std::vector<CID> commds;
    };
  };
  CBOR_TUPLE(ComputeDataCommitment::Params, inputs)
  CBOR_TUPLE(ComputeDataCommitment::Result, commds)

}  // namespace fc::vm::actor::builtin::v5::market
