/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_INDICES_INDICES_HPP
#define CPP_FILECOIN_VM_INDICES_INDICES_HPP

#include "power/power_table.hpp"
#include "primitives/types.hpp"

namespace fc::vm::indices {
  using primitives::SectorStorageWeightDesc;

  class Indices {
   public:
    virtual ~Indices() = default;

    // TODO(artyom-yurin): [FIL-135] FROM SPEC: The function is located in the
    // indices module temporarily, until we find a better place for global
    // parameterization functions.
    /**
     * @brief Get power of sector
     * @param storage_weight_desc is description of sector
     * @return power of sector
     */
    virtual fc::power::Power consensusPowerForStorageWeight(
        SectorStorageWeightDesc storage_weight_desc) = 0;

    /**
     * @brief Get min power to participate in consensus
     * @return min power
     */
    virtual fc::power::Power storagePowerConsensusMinMinerPower() = 0;
  };

}  // namespace fc::vm::indices

#endif  // CPP_FILECOIN_VM_INDICES_INDICES_HPP
