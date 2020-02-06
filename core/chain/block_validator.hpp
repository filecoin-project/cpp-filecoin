/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CHAIN_BLOCK_VALIDATOR_HPP
#define CPP_FILECOIN_CORE_CHAIN_BLOCK_VALIDATOR_HPP

#include "primitives/block/block.hpp"

namespace fc::chain::block_validator {

  class BlockValidator {
   public:
    using BlockHeader = primitives::block::BlockHeader;
    virtual ~BlockValidator() = default;

    virtual outcome::result<void> validateBlock(
        const BlockHeader &block) const = 0;
  };

}  // namespace fc::chain::block_validator

#endif  // CPP_FILECOIN_CORE_CHAIN_BLOCK_VALIDATOR_HPP
