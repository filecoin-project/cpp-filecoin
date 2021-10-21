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
  using primitives::DealId;

  struct SectorDeals {
    ChainEpoch sector_expiry{};
    std::vector<DealId> deal_ids;
  };
  CBOR_TUPLE(SectorDeals, sector_expiry, deal_ids)

}  // namespace fc::vm::actor::builtin::types::market
