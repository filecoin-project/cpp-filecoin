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

    virtual fc::primitives::BigInt consensusPowerForStorageWeight(
        actor::SectorStorageWeightDesc storage_weight_desc) = 0;

    virtual fc::primitives::BigInt storagePowerConsensusMinMinerPower() = 0;
  };
}  // namespace fc::vm

#endif  // CPP_FILECOIN_CORE_VM_INDICES_HPP
