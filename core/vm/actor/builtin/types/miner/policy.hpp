/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "const.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using primitives::sector::RegisteredSealProof;

  /**
   * The period over which all a miner's active sectors will be challenged.
   * 24 hours
   */
  static ChainEpoch kWPoStProvingPeriod = kEpochsInDay;

  /**
   * The duration of a deadline's challenge window, the period before a deadline
   * when the challenge is available.
   * 30 minutes (48 per day)
   */
  static EpochDuration kWPoStChallengeWindow = 30 * 60 / kEpochDurationSeconds;

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
  static EpochDuration kFaultMaxAge = kWPoStProvingPeriod * 14;

  constexpr auto kWorkerKeyChangeDelay{2 * kElectionLookback};

  static auto kMinSectorExpiration = 180 * kEpochsInDay;

  constexpr auto kAddressedSectorsMax{10000};

  /**
   * List of proof types which can be used when creating new miner actors
   */
  static std::set<RegisteredSealProof> kSupportedProofs{
      RegisteredSealProof::kStackedDrg32GiBV1,
      RegisteredSealProof::kStackedDrg64GiBV1,
  };

  /**
   * Maximum number of epochs past the current epoch a sector may be set to
   * expire. The actual maximum extension will be the minimum of CurrEpoch +
   * MaximumSectorExpirationExtension and
   * sector.ActivationEpoch+sealProof.SectorMaximumLifetime()
   */
  static auto kMaxSectorExpirationExtension = 540 * kEpochsInDay;

  inline outcome::result<EpochDuration> maxSealDuration(
      RegisteredSealProof type) {
    switch (type) {
      case RegisteredSealProof::kStackedDrg32GiBV1:
      case RegisteredSealProof::kStackedDrg2KiBV1:
      case RegisteredSealProof::kStackedDrg8MiBV1:
      case RegisteredSealProof::kStackedDrg512MiBV1:
      case RegisteredSealProof::kStackedDrg64GiBV1:
        return 10000;
      default:
        return VMExitCode::kErrIllegalArgument;
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

  /** Maximum number of control addresses a miner may register. */
  constexpr size_t kMaxControlAddresses = 10;

  /**
   * List of proof types which may be used when creating a new miner actor or
   * pre-committing a new sector.
   */
  static const std::set<RegisteredSealProof> kPreCommitSealProofTypesV0{
      RegisteredSealProof::kStackedDrg32GiBV1,
      RegisteredSealProof::kStackedDrg64GiBV1,
  };

  static const std::set<RegisteredSealProof> kPreCommitSealProofTypesV7{
      RegisteredSealProof::kStackedDrg32GiBV1,
      RegisteredSealProof::kStackedDrg64GiBV1,
      RegisteredSealProof::kStackedDrg32GiBV1_1,
      RegisteredSealProof::kStackedDrg64GiBV1_1,
  };

  /**
   * From network version 8, sectors sealed with the V1 seal proof types cannot
   * be committed
   */
  static const std::set<RegisteredSealProof> kPreCommitSealProofTypesV8{
      RegisteredSealProof::kStackedDrg32GiBV1_1,
      RegisteredSealProof::kStackedDrg64GiBV1_1,
  };

  /// Libp2p peer info limits.
  /**
   * MaxPeerIDLength is the maximum length allowed for any on-chain peer ID.
   * Most Peer IDs are expected to be less than 50 bytes.
   */
  constexpr size_t kMaxPeerIDLength = 128;

  /**
   * MaxMultiaddrData is the maximum amount of data that can be stored in
   * multiaddrs.
   */
  constexpr size_t kMaxMultiaddressData = 1024;

  static primitives::EpochDuration kMaxProveCommitDuration =
      kEpochsInDay + kPreCommitChallengeDelay;

  /**
   * Sets miner policy parameters
   * @param epochDurationSeconds - block delay in seconds
   * @param supportedProofs - proof types supported by miner
   */
  inline void setPolicy(size_t epochDurationSeconds,
                        std::set<RegisteredSealProof> supportedProofs) {
    kWPoStChallengeWindow = 30 * 60 / kEpochDurationSeconds;

    const auto epochsInHour = kSecondsInHour / epochDurationSeconds;
    const auto epochsInDay = 24 * epochsInHour;
    kWPoStProvingPeriod = epochsInDay;

    kFaultMaxAge = kWPoStProvingPeriod * 14;
    kMinSectorExpiration = 180 * epochsInDay;
    kMaxSectorExpirationExtension = 540 * epochsInDay;
    kMaxProveCommitDuration = epochsInDay + kPreCommitChallengeDelay;

    kSupportedProofs = std::move(supportedProofs);
  }
}  // namespace fc::vm::actor::builtin::types::miner
