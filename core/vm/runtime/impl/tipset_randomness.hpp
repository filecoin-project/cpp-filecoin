/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "fwd.hpp"
#include "vm/runtime/runtime_randomness.hpp"

namespace fc::vm::runtime {
  class TipsetRandomness : public RuntimeRandomness {
   public:
    TipsetRandomness(TsLoadPtr ts_load, SharedMutexPtr ts_branches_mutex);

    outcome::result<Randomness> getRandomnessFromTickets(
        const TsBranchPtr &ts_branch,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;

    outcome::result<Randomness> getRandomnessFromBeacon(
        const TsBranchPtr &ts_branch,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;

   private:
    TsLoadPtr ts_load;
    SharedMutexPtr ts_branches_mutex;
  };

}  // namespace fc::vm::runtime
