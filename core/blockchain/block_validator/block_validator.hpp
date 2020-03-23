/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CHAIN_BLOCK_VALIDATOR_HPP
#define CPP_FILECOIN_CORE_CHAIN_BLOCK_VALIDATOR_HPP

#include "common/outcome.hpp"

#include "blockchain/block_validator/block_validator_scenarios.hpp"
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
    virtual outcome::result<void> validateBlock(const BlockHeader &header,
                                                scenarios::Scenario scenario) const = 0;
  };

}  // namespace fc::blockchain::block_validator

#endif  // CPP_FILECOIN_CORE_CHAIN_BLOCK_VALIDATOR_HPP
