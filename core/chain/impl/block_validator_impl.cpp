/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "chain/impl/block_validator_impl.hpp"

namespace fc::chain::block_validator {
  // TODO (yuraz): FIL-87 implement proper validation
  outcome::result<void> BlockValidatorImpl::validateBlock(
      const BlockHeader &block) const {
    return outcome::success();
  }

}  // namespace fc::chain::block_validator
