/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CHAIN_IMPL_WEIGHT_CALCULATOR_IMPL_HPP
#define CPP_FILECOIN_CORE_CHAIN_IMPL_WEIGHT_CALCULATOR_IMPL_HPP

#include "blockchain/weight_calculator.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::blockchain::weight {
  using Ipld = storage::ipfs::IpfsDatastore;

  enum class WeightCalculatorError { NO_NETWORK_POWER = 1 };

  class WeightCalculatorImpl : public WeightCalculator {
   public:
    WeightCalculatorImpl(std::shared_ptr<Ipld> ipld);

    ~WeightCalculatorImpl() override = default;

    outcome::result<BigInt> calculateWeight(const Tipset &tipset) override;

   private:
    std::shared_ptr<Ipld> ipld_;
  };

}  // namespace fc::blockchain::weight

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::weight, WeightCalculatorError);

#endif  // CPP_FILECOIN_CORE_CHAIN_IMPL_WEIGHT_CALCULATOR_IMPL_HPP
