/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_TIPSET_RANDOMNESS_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_TIPSET_RANDOMNESS_HPP

#include "primitives/tipset/tipset.hpp"
#include "vm/runtime/runtime_randomness.hpp"

namespace fc::vm::runtime {
  using primitives::tipset::TipsetCPtr;

  class TipsetRandomness : public RuntimeRandomness {
   public:
    TipsetRandomness(std::shared_ptr<Ipld> ipld, TipsetCPtr tipset);

    outcome::result<Randomness> getRandomnessFromTickets(
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;

    outcome::result<Randomness> getRandomnessFromBeacon(
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;

   private:
    std::shared_ptr<Ipld> ipld_;
    TipsetCPtr tipset_;
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_TIPSET_RANDOMNESS_HPP
