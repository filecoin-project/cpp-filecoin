/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INDICES_HPP
#define CPP_FILECOIN_CORE_VM_INDICES_HPP

#include "primitives/big_int.hpp"
#include "vm/actor/util.hpp"

namespace fc::vm {
  class Indices {
   public:

    virtual ~Indices() = default;

    fc::primitives::BigInt consensusPowerForStorageWeight(
        actor::SectorStorageWeightDesc storage_weight_desc);

    fc::primitives::BigInt storagePowerConsensusMinMinerPower();
  };
}  // namespace fc::vm

#endif  // CPP_FILECOIN_CORE_VM_INDICES_HPP
