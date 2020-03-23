/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TESTUTIL_BLOCK_VALIDATOR_MOCK_HPP
#define CPP_FILECOIN_TESTUTIL_BLOCK_VALIDATOR_MOCK_HPP

#include <gmock/gmock.h>

#include "blockchain/block_validator/block_validator.hpp"

namespace fc::blockchain::block_validator {
  class BlockValidatorMock : public BlockValidator {
   public:
    MOCK_CONST_METHOD2(validateBlock,
                       outcome::result<void>(const BlockHeader &block,
                                             scenarios::Scenario scenario));
  };
}  // namespace fc::blockchain::block_validator

#endif
