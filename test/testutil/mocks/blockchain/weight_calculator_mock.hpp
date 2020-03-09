/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MOCKS_BLOCKCHAIN_WEIGHT_CALCULATOR
#define CPP_FILECOIN_MOCKS_BLOCKCHAIN_WEIGHT_CALCULATOR

#include <gmock/gmock.h>

#include "filecoin/blockchain/weight_calculator.hpp"

namespace fc::blockchain::weight {
  class WeightCalculatorMock : public WeightCalculator {
   public:
    MOCK_METHOD1(calculateWeight,
                 outcome::result<BigInt>(const Tipset &tipset));
  };
}  // namespace fc::blockchain::weight

#endif
