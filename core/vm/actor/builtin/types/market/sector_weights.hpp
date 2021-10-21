/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_codec.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::market {
  using primitives::ChainEpoch;
  using primitives::DealWeight;

  struct SectorWeights {
    uint64_t deal_space{};
    DealWeight deal_weight;
    DealWeight verified_deal_weight;
  };
  CBOR_TUPLE(SectorWeights, deal_space, deal_weight, verified_deal_weight)

}  // namespace fc::vm::actor::builtin::types::market
