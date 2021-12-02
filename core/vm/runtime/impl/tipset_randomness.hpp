/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "fwd.hpp"
#include "vm/runtime/runtime_randomness.hpp"

namespace fc::vm::runtime {
  using drand::DrandSchedule;

  class TipsetRandomness : public RuntimeRandomness {
   public:
    TipsetRandomness(TsLoadPtr ts_load,
                     SharedMutexPtr ts_branches_mutex,
                     std::shared_ptr<DrandSchedule> drand_schedule);

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
    std::shared_ptr<DrandSchedule> drand_schedule;
  };

}  // namespace fc::vm::runtime
