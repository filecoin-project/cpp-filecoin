/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TYPES_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TYPES_HPP

#include <cstdint>

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::primitives {
  using TokenAmount = BigInt;

  using SectorSize = uint64_t;

  using SectorNumber = uint64_t;

  using DealWeight = BigInt;

  using DealId = uint64_t;

  struct SectorStorageWeightDesc {
    SectorSize sector_size;
    EpochDuration duration;
    DealWeight deal_weight;
  };

  CBOR_TUPLE(SectorStorageWeightDesc, sector_size, duration, deal_weight)
}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TYPES_HPP
