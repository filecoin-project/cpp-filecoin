/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/big_int.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::blockchain::weight {
  /**
   * @class WeightCalculator is an interface providing a method for calculating
   * tipset weight
   */
  class WeightCalculator {
   public:
    using BigInt = primitives::BigInt;
    using Tipset = primitives::tipset::Tipset;

    virtual ~WeightCalculator() = default;

    virtual outcome::result<BigInt> calculateWeight(const Tipset &tipset) = 0;
  };
}  // namespace fc::blockchain::weight
