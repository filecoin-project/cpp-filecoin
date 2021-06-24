/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "const.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"
#include "vm/actor/builtin/types/miner/types.hpp"
#include "vm/actor/builtin/types/shared.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using libp2p::multi::HashType;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::DealWeight;
  using primitives::EpochDuration;
  using primitives::SectorQuality;
  using primitives::SectorSize;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::cid::kCommitmentBytesLen;
  using primitives::sector::RegisteredSealProof;

  // The period over which all a miner's active sectors will be challenged.
  // 24 hours
  extern ChainEpoch kWPoStProvingPeriod;

  // The duration of a deadline's challenge window, the period before a deadline
  // when the challenge is available. 30 minutes (48 per day)
  extern EpochDuration kWPoStChallengeWindow;

  // The number of non-overlapping PoSt deadlines in each proving period.
  constexpr size_t kWPoStPeriodDeadlines{48};

  // MaxPartitionsPerDeadline is the maximum number of partitions that will be
  // assigned to a deadline. For a minimum storage of upto 1Eib, we need 300
  // partitions per deadline. 48 * 32GiB * 2349 * 300 = 1.00808144 EiB So, to
  // support upto 10Eib storage, we set this to 3000.
  constexpr uint64_t kMaxPartitionsPerDeadline{3000};

  constexpr size_t kSectorsMax{32 << 20};
  constexpr uint64_t kAddressedSectorsMax{10000};

  constexpr size_t kNewSectorsPerPeriodMax{128 << 10};
  constexpr EpochDuration kChainFinality{900};

  inline const CidPrefix kSealedCIDPrefix{
      .version = static_cast<uint64_t>(CID::Version::V1),
      .codec =
          static_cast<uint64_t>(CID::Multicodec::FILECOIN_COMMITMENT_SEALED),
      .mh_type = HashType::poseidon_bls12_381_a1_fc1,
      .mh_length = kCommitmentBytesLen};

  // Number of epochs between publishing the precommit and when the challenge
  // for interactive PoRep is drawn used to ensure it is not predictable by
  // miner
  constexpr EpochDuration kPreCommitChallengeDelay{150};

  // Lookback from the current epoch for state view for leader elections.
  constexpr EpochDuration kElectionLookback{1};

  // Lookback from the deadline's challenge window opening from which to sample
  // chain randomness for the challenge seed. This lookback exists so that
  // deadline windows can be non-overlapping (which make the programming
  // simpler) but without making the miner wait for chain stability before being
  // able to start on PoSt computation. The challenge is available this many
  // epochs before the window is actually open to receiving a PoSt.
  constexpr EpochDuration kWPoStChallengeLookback{20};

  // Minimum period before a deadline's challenge window opens that a fault must
  // be declared for that deadline. This lookback must not be less than
  // WPoStChallengeLookback lest a malicious miner be able to selectively
  // declare faults after learning the challenge value.
  constexpr EpochDuration kFaultDeclarationCutoff{kWPoStChallengeLookback + 50};

  // The maximum age of a fault before the sector is terminated.
  extern EpochDuration kFaultMaxAge;

  constexpr auto kWorkerKeyChangeDelay{kChainFinality};

  // Minimum number of epochs past the current epoch a sector may be set to
  // expire.
  extern ChainEpoch kMinSectorExpiration;

  // Maximum number of epochs past the current epoch a sector may be set to
  // expire. The actual maximum extension will be the minimum of CurrEpoch +
  // MaximumSectorExpirationExtension and
  // sector.ActivationEpoch+sealProof.SectorMaximumLifetime()
  extern ChainEpoch kMaxSectorExpirationExtension;

  // List of proof types which can be used when creating new miner actors
  extern std::set<RegisteredSealProof> kSupportedProofs;

  // Ratio of sector size to maximum number of deals per sector.
  // The maximum number of deals is the sector size divided by this number
  // (2^27) which limits 32GiB sectors to 256 deals and 64GiB sectors to 512
  constexpr uint64_t kDealLimitDenominator{134217728};

  // Number of epochs after a consensus fault for which a miner is ineligible
  // for permissioned actor methods and winning block elections.
  constexpr auto kConsensusFaultIneligibilityDuration{kChainFinality};

  // WPoStDisputeWindow is the period after a challenge window ends during which
  // PoSts submitted during that period may be disputed.
  constexpr auto kWPoStDisputeWindow{2 * kChainFinality};

  constexpr auto kMaxSectorNumber{INT64_MAX};

  inline SectorQuality qualityForWeight(SectorSize size,
                                        ChainEpoch duration,
                                        const DealWeight &deal_weight,
                                        const DealWeight &verified_weight) {
    const auto sector_space_time = size * duration;
    const auto total_deal_space_time = deal_weight * verified_weight;
    assert(sector_space_time >= total_deal_space_time);

    const auto weighted_base_space_time =
        (sector_space_time - total_deal_space_time) * kQualityBaseMultiplier;
    const auto weighted_deal_space_time = deal_weight * kDealWeightMultiplier;
    const auto weighted_verified_space_time =
        verified_weight * kVerifiedDealWeightMultiplier;
    const auto weighted_sum_space_time = weighted_base_space_time
                                         + weighted_deal_space_time
                                         + weighted_verified_space_time;
    const auto scaled_up_weighted_sum_space_time = weighted_sum_space_time
                                                   << kSectorQualityPrecision;

    return bigdiv(bigdiv(scaled_up_weighted_sum_space_time, sector_space_time),
                  kQualityBaseMultiplier);
  }

  inline StoragePower qaPowerForWeight(SectorSize size,
                                       ChainEpoch duration,
                                       const DealWeight &deal_weight,
                                       const DealWeight &verified_weight) {
    const auto quality =
        qualityForWeight(size, duration, deal_weight, verified_weight);
    return (size * quality) >> kSectorQualityPrecision;
  }

  inline StoragePower qaPowerForSector(SectorSize size,
                                       const SectorOnChainInfo &sector) {
    const auto duration = sector.expiration - sector.activation_epoch;
    return qaPowerForWeight(
        size, duration, sector.deal_weight, sector.verified_deal_weight);
  }

  inline uint64_t sectorDealsMax(SectorSize size) {
    return std::max(static_cast<uint64_t>(256), size / kDealLimitDenominator);
  }

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

  inline TokenAmount rewardForConsensusSlashReport(
      EpochDuration age, const TokenAmount &collateral) {
    const auto init{std::make_pair(1, 1000)};
    const auto grow{std::make_pair(TokenAmount{101251}, TokenAmount{100000})};
    using boost::multiprecision::pow;
    return std::min(bigdiv(collateral, 2),
                    bigdiv(collateral * init.first * pow(grow.first, age),
                           init.second * pow(grow.second, age)));
  }

  inline PowerPair powerForSectors(SectorSize ssize, const std::vector<SectorOnChainInfo> &sectors) {
    StoragePower qa{0};

    for (const auto &sector : sectors) {
      qa += qaPowerForSector(ssize, sector);
    }

    return PowerPair(ssize * sectors.size(), qa);
  }

  // Maximum number of control addresses a miner may register.
  constexpr size_t kMaxControlAddresses = 10;

  // List of proof types which may be used when creating a new miner actor or
  // pre-committing a new sector.
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

  // From network version 8, sectors sealed with the V1 seal proof types cannot
  // be committed
  static const std::set<RegisteredSealProof> kPreCommitSealProofTypesV8{
      RegisteredSealProof::kStackedDrg32GiBV1_1,
      RegisteredSealProof::kStackedDrg64GiBV1_1,
  };

  // MaxPeerIDLength is the maximum length allowed for any on-chain peer ID.
  // Most Peer IDs are expected to be less than 50 bytes.
  constexpr size_t kMaxPeerIDLength = 128;

  // MaxMultiaddrData is the maximum amount of data that can be stored in
  // multiaddrs.
  constexpr size_t kMaxMultiaddressData = 1024;

  // Maximum size of a single prove-commit proof, in bytes.
  // The 1024 maximum at network version 4 was an error (the expected size is
  // 1920).
  constexpr size_t kMaxProveCommitSizeV4{1024};
  constexpr size_t kMaxProveCommitSizeV5{10240};

  // Maximum size of a single prove-commit proof, in bytes (the expected size is
  // 192).
  constexpr size_t kMaxPoStProofSize{1024};

  extern EpochDuration kMaxProveCommitDuration;

  struct VestSpec {
    ChainEpoch initial_delay{};
    ChainEpoch vest_period{};
    ChainEpoch step_duration{};
    ChainEpoch quantization{};
  };

  const VestSpec kRewardVestingSpecV0{
      .initial_delay = static_cast<ChainEpoch>(20 * kEpochsInDay),
      .vest_period = static_cast<ChainEpoch>(180 * kEpochsInDay),
      .step_duration = static_cast<ChainEpoch>(kEpochsInDay),
      .quantization = static_cast<ChainEpoch>(12 * kEpochsInHour)};

  const VestSpec kRewardVestingSpecV1{
      .initial_delay = 0,
      .vest_period = static_cast<ChainEpoch>(180 * kEpochsInDay),
      .step_duration = static_cast<ChainEpoch>(kEpochsInDay),
      .quantization = static_cast<ChainEpoch>(12 * kEpochsInHour)};

}  // namespace fc::vm::actor::builtin::types::miner
