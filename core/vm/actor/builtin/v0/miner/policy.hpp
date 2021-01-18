/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::SectorSize;
  using primitives::SectorStorageWeightDesc;
  using primitives::TokenAmount;
  using primitives::sector::RegisteredProof;
  using primitives::sector::RegisteredSealProof;

  constexpr size_t kEpochDurationSeconds{30};
  constexpr size_t kSecondsInHour{3600};
  constexpr size_t kSecondsInDay{86400};
  constexpr size_t kSecondsInYear{31556925};
  constexpr size_t kEpochsInHour{kSecondsInHour / kEpochDurationSeconds};
  constexpr size_t kEpochsInDay{kSecondsInDay / kEpochDurationSeconds};
  constexpr size_t kEpochsInYear{kSecondsInYear / kEpochDurationSeconds};

  /**
   * The period over which all a miner's active sectors will be challenged.
   * 24 hours
   */
  constexpr ChainEpoch kWPoStProvingPeriod{kEpochsInDay};

  /**
   * The duration of a deadline's challenge window, the period before a deadline
   * when the challenge is available.
   * 30 minutes (48 per day)
   */
  constexpr EpochDuration kWPoStChallengeWindow{30 * 60
                                                / kEpochDurationSeconds};

  /** The number of non-overlapping PoSt deadlines in each proving period. */
  constexpr size_t kWPoStPeriodDeadlines{48};

  constexpr size_t kSectorsMax{32 << 20};
  constexpr size_t kNewSectorsPerPeriodMax{128 << 10};
  constexpr EpochDuration kChainFinalityish{900};

  /**
   * Number of epochs between publishing the precommit and when the challenge
   * for interactive PoRep is drawn used to ensure it is not predictable by
   * miner
   */
  constexpr EpochDuration kPreCommitChallengeDelay{150};

  /** Lookback from the current epoch for state view for leader elections. */
  constexpr EpochDuration kElectionLookback{1};

  /**
   * Lookback from the deadline's challenge window opening from which to sample
   * chain randomness for the challenge seed. This lookback exists so that
   * deadline windows can be non-overlapping (which make the programming
   * simpler) but without making the miner wait for chain stability before being
   * able to start on PoSt computation. The challenge is available this many
   * epochs before the window is actually open to receiving a PoSt.
   */
  constexpr EpochDuration kWPoStChallengeLookback{20};

  /**
   * Minimum period before a deadline's challenge window opens that a fault must
   * be declared for that deadline. This lookback must not be less than
   * WPoStChallengeLookback lest a malicious miner be able to selectively
   * declare faults after learning the challenge value.
   */
  constexpr EpochDuration kFaultDeclarationCutoff{kWPoStChallengeLookback + 50};

  /** The maximum age of a fault before the sector is terminated. */
  constexpr EpochDuration kFaultMaxAge{kWPoStProvingPeriod * 14};

  constexpr auto kWorkerKeyChangeDelay{2 * kElectionLookback};

  constexpr auto kMinSectorExpiration = 180 * kEpochsInDay;

  constexpr auto kAddressedSectorsMax{10000};

  /**
   * List of proof types which can be used when creating new miner actors
   */
  static const std::set<RegisteredSealProof> kSupportedProofs{
      RegisteredSealProof::StackedDrg32GiBV1,
      RegisteredSealProof::StackedDrg64GiBV1,
  };

  /**
   * Maximum number of epochs past the current epoch a sector may be set to
   * expire. The actual maximum extension will be the minimum of CurrEpoch +
   * MaximumSectorExpirationExtension and
   * sector.ActivationEpoch+sealProof.SectorMaximumLifetime()
   */
  constexpr auto kMaxSectorExpirationExtension = 540 * kEpochsInDay;

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
}  // namespace fc::vm::actor::builtin::v0::miner
