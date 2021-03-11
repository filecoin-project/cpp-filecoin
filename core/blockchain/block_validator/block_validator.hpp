/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/block_validator/block_validator_scenarios.hpp"
#include "common/outcome.hpp"
#include "primitives/block/block.hpp"

namespace fc::blockchain::block_validator {

  /**
   * @interface Block validator
   */
  class BlockValidator {
   public:
    using BlockHeader = primitives::block::BlockHeader;

    virtual ~BlockValidator() = default;

    /**
     * @brief Validate block header
     * @param header - header to validate
     * @param scenario - required validation stages
     * @return validation result
     */
    virtual outcome::result<void> validateBlock(
        const BlockHeader &header, scenarios::Scenario scenario) const = 0;
  };

}  // namespace fc::blockchain::block_validator
