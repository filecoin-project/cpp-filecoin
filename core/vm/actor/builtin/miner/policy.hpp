/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_POLICY_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_POLICY_HPP

#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::miner {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::SectorSize;
  using primitives::SectorStorageWeightDesc;
  using primitives::TokenAmount;

  constexpr ChainEpoch kPoStLookback{1};

  constexpr auto kElectionLookback{kPoStLookback};

  constexpr size_t kNumWindowedPoStSectors{200};

  constexpr auto kWorkerKeyChangeDelay{2 * kElectionLookback};

  constexpr EpochDuration kProvingPeriod{300};

  constexpr EpochDuration kPreCommitChallengeDelay{10};

  constexpr EpochDuration kChainFinalityish{500};

  constexpr EpochDuration kDeclaredFaultEffectiveDelay{20};

  inline TokenAmount precommitDeposit(SectorSize sector_size,
                                      ChainEpoch duration) {
    return 0;
  }

  inline TokenAmount temporaryFaultFee(
      const std::vector<SectorStorageWeightDesc> &weights,
      EpochDuration duration) {
    return 0;
  }
}  // namespace fc::vm::actor::builtin::miner

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_POLICY_HPP
