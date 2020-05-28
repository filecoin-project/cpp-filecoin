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
  using ActorId = uint64_t;

  using TokenAmount = BigInt;

  using TipsetWeight = BigInt;

  using SectorSize = uint64_t;

  using SectorNumber = uint64_t;

  using DealWeight = BigInt;

  using DealId = uint64_t;

  using GasAmount = int64_t;

  // StorageID identifies sector storage by UUID. One sector storage should map
  // to one
  //  filesystem, local or networked / shared by multiple machines
  using StorageID = std::string;

  struct FsStat {
    uint64_t capacity;
    uint64_t available;  // Available to use for sector storage
    uint64_t used;
  };

  struct SectorStorageWeightDesc {
    SectorSize sector_size;
    EpochDuration duration;
    DealWeight deal_weight;
  };

  struct PieceDescriptor {
    SectorNumber id;
    uint64_t offset;
    uint64_t length;
  };

  using StoragePower = primitives::BigInt;

  CBOR_TUPLE(SectorStorageWeightDesc, sector_size, duration, deal_weight)
}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TYPES_HPP
