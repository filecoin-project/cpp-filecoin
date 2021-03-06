/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/weight_calculator.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::blockchain::weight {
  enum class WeightCalculatorError { kNoNetworkPower = 1 };

  class WeightCalculatorImpl : public WeightCalculator {
   public:
    explicit WeightCalculatorImpl(std::shared_ptr<Ipld> ipld);

    ~WeightCalculatorImpl() override = default;

    outcome::result<BigInt> calculateWeight(const Tipset &tipset) override;

   private:
    std::shared_ptr<Ipld> ipld_;
  };

}  // namespace fc::blockchain::weight

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::weight, WeightCalculatorError);
