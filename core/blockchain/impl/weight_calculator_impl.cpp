/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/weight_calculator_impl.hpp"

namespace fc::blockchain::weight {
  using primitives::BigInt;

  outcome::result<BigInt> WeightCalculatorImpl::calculateWeight(
      const Tipset &tipset) {
    return 1;  // this value is fake, make proper implementation
  }

}  // namespace fc::blockchain::weight
