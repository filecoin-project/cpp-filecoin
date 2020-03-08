/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/blockchain/impl/weight_calculator_impl.hpp"

namespace fc::blockchain::weight {
  using primitives::BigInt;

  outcome::result<BigInt> WeightCalculatorImpl::calculateWeight(
      const Tipset &tipset) {
    // TODO(yuraz): FIL-155 implement weight calculator, this one is stub
    return 1;
  }

}  // namespace fc::blockchain::weight
