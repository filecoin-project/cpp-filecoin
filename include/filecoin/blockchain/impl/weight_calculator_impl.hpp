/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CHAIN_IMPL_WEIGHT_CALCULATOR_IMPL_HPP
#define CPP_FILECOIN_CORE_CHAIN_IMPL_WEIGHT_CALCULATOR_IMPL_HPP

#include "filecoin/blockchain/weight_calculator.hpp"

namespace fc::blockchain::weight {

  // TODO(yuraz): FIL-155 implement weight calculator, this one is stub
  class WeightCalculatorImpl : public WeightCalculator {
   public:
    ~WeightCalculatorImpl() override = default;

    outcome::result<BigInt> calculateWeight(const Tipset &tipset) override;
  };

}  // namespace fc::blockchain::weight

#endif  // CPP_FILECOIN_CORE_CHAIN_IMPL_WEIGHT_CALCULATOR_IMPL_HPP
