/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_POLICY_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_POLICY_HPP

#include "common/outcome.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::miner {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::SectorSize;
  using primitives::SectorStorageWeightDesc;
  using primitives::TokenAmount;
  using primitives::sector::RegisteredProof;

  constexpr size_t kEpochDurationSeconds{25};
  constexpr size_t kSecondsInHour{3600};
  constexpr size_t kSecondsInDay{86400};
  constexpr size_t kSecondsInYear{31556925};
  constexpr size_t kEpochsInHour{kSecondsInHour / kEpochDurationSeconds};
  constexpr size_t kEpochsInDay{kSecondsInDay / kEpochDurationSeconds};
  constexpr size_t kEpochsInYear{kSecondsInYear / kEpochDurationSeconds};

  constexpr ChainEpoch kWPoStProvingPeriod{kEpochsInDay};
  constexpr EpochDuration kWPoStChallengeWindow{3600 / kEpochDurationSeconds};
  constexpr size_t kWPoStPeriodDeadlines{kWPoStProvingPeriod
                                         / kWPoStChallengeWindow};
  constexpr size_t kSectorsMax{32 << 20};
  constexpr size_t kNewSectorsPerPeriodMax{128 << 10};
  constexpr EpochDuration kChainFinalityish{500};
  constexpr EpochDuration kPreCommitChallengeDelay{10};
  constexpr EpochDuration kElectionLookback{1};
  constexpr EpochDuration kWPoStChallengeLookback{20};
  constexpr EpochDuration kFaultDeclarationCutoff{kWPoStChallengeLookback};
  constexpr EpochDuration kFaultMaxAge{kWPoStProvingPeriod * 14 - 1};
  constexpr auto kWorkerKeyChangeDelay{2 * kElectionLookback};

  inline outcome::result<EpochDuration> maxSealDuration(RegisteredProof type) {
    switch (type) {
      case RegisteredProof::StackedDRG32GiBSeal:
      case RegisteredProof::StackedDRG2KiBSeal:
      case RegisteredProof::StackedDRG8MiBSeal:
      case RegisteredProof::StackedDRG512MiBSeal:
      case RegisteredProof::StackedDRG64GiBSeal:
        return 10000;
      default:
        return VMExitCode::kMinerActorIllegalArgument;
    }
  }

  inline auto windowPoStMessagePartitionsMax(uint64_t partitions) {
    return 100000 / partitions;
  }

  inline TokenAmount precommitDeposit(SectorSize sector_size,
                                      ChainEpoch duration) {
    return 0;
  }

  inline TokenAmount rewardForConsensusSlashReport(EpochDuration age,
                                                   TokenAmount collateral) {
    const auto init{std::make_pair(1, 1000)};
    const auto grow{std::make_pair(TokenAmount{101251}, TokenAmount{100000})};
    using boost::multiprecision::pow;
    return std::min(bigdiv(collateral, 2),
                    bigdiv(collateral * init.first * pow(grow.first, age),
                           init.second * pow(grow.second, age)));
  }
}  // namespace fc::vm::actor::builtin::miner

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_POLICY_HPP
