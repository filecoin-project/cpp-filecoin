/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_UTIL_HPP
#define CPP_FILECOIN_VM_ACTOR_UTIL_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::vm::actor {

  using primitives::EpochDuration;
  using Weight = primitives::BigInt;
  using PeerId = std::string;
  using TokenAmount = primitives::BigInt;

  struct SectorStorageWeightDescr {
    uint64_t sector_size;
    EpochDuration duration;
    Weight deal_weight;
  };

  CBOR_TUPLE(SectorStorageWeightDescr, sector_size, duration, deal_weight)

}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_VM_ACTOR_UTIL_HPP
