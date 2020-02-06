/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CHAIN_IMPL_BLOCK_VALIDATOR_IMPL_HPP
#define CPP_FILECOIN_CORE_CHAIN_IMPL_BLOCK_VALIDATOR_IMPL_HPP

#include "chain/block_validator.hpp"

namespace fc::chain::block_validator {

  class BlockValidatorImpl : public BlockValidator {
   public:
    ~BlockValidatorImpl() override = default;

    outcome::result<void> validateBlock(
        const BlockHeader &block) const override;
  };

}  // namespace fc::chain::block_validator

#endif  // CPP_FILECOIN_CORE_CHAIN_IMPL_BLOCK_VALIDATOR_IMPL_HPP
