/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "const.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

#define DEFINE(x) \
  decltype(x) x  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

namespace fc {
  using primitives::sector::RegisteredSealProof;
  using vm::actor::builtin::types::miner::kPreCommitChallengeDelay;

  // Initialize parameters with mainnet values
  DEFINE(kEpochDurationSeconds){30};
  DEFINE(kEpochsInHour){kSecondsInHour / kEpochDurationSeconds};
  DEFINE(kEpochsInDay){24 * kEpochsInHour};
  DEFINE(kEpochsInYear){365 * kEpochsInDay};

  DEFINE(kPropagationDelaySecs){6};

  DEFINE(kUpgradeBreezeHeight){41280};
  DEFINE(kUpgradeSmokeHeight){51000};
  DEFINE(kUpgradeIgnitionHeight){94000};
  DEFINE(kUpgradeRefuelHeight){130800};
  DEFINE(kUpgradeActorsV2Height){138720};
  DEFINE(kUpgradeTapeHeight){140760};
  DEFINE(kUpgradeLiftoffHeight){148888};
  DEFINE(kUpgradeKumquatHeight){170000};
  DEFINE(kUpgradeCalicoHeight){265200};
  DEFINE(kUpgradePersianHeight){272400};
  DEFINE(kUpgradeOrangeHeight){336458};
  DEFINE(kUpgradeClausHeight){343200};
  DEFINE(kUpgradeActorsV3Height){550321};
  DEFINE(kUpgradeNorwegianHeight){665280};
  DEFINE(kUpgradeActorsV4Height){712320};
  DEFINE(kUpgradeHyperdriveHeight){892800};

  DEFINE(kBreezeGasTampingDuration){120};

  DEFINE(kInteractivePoRepConfidence){6};
}  // namespace fc

namespace fc::vm::actor::builtin::types::market {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  EpochDuration kDealUpdatesInterval{static_cast<EpochDuration>(kEpochsInDay)};
}  // namespace fc::vm::actor::builtin::types::market

namespace fc::vm::actor::builtin::types::miner {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  ChainEpoch kWPoStProvingPeriod = kEpochsInDay;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  EpochDuration kWPoStChallengeWindow = 30 * 60 / kEpochDurationSeconds;
  DEFINE(kPreCommitChallengeDelay){150};
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  EpochDuration kFaultMaxAge{kWPoStProvingPeriod * 14};
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  ChainEpoch kMinSectorExpiration = 180 * kEpochsInDay;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  std::set<RegisteredSealProof> kSupportedProofs{
      RegisteredSealProof::kStackedDrg32GiBV1,
      RegisteredSealProof::kStackedDrg64GiBV1,
  };
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  ChainEpoch kMaxSectorExpirationExtension = 540 * kEpochsInDay;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  EpochDuration kMaxProveCommitDuration =
      kEpochsInDay + kPreCommitChallengeDelay;
}  // namespace fc::vm::actor::builtin::types::miner

namespace fc::vm::actor::builtin::types::payment_channel {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  EpochDuration kSettleDelay = kEpochsInHour * 12;
}

namespace fc::vm::actor::builtin::types::storage_power {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  StoragePower kConsensusMinerMinPower{StoragePower{10} << 40};
}

namespace fc::vm::actor::builtin::types::verified_registry {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  StoragePower kMinVerifiedDealSize{StoragePower{1} << 20};
}

namespace fc {
  void setParams2K() {
    kEpochDurationSeconds = 4;
    kEpochsInHour = kSecondsInHour / kEpochDurationSeconds;
    kEpochsInDay = 24 * kEpochsInHour;
    kEpochsInYear = 365 * kEpochsInDay;

    kPropagationDelaySecs = 1;

    // Network versions
    kUpgradeBreezeHeight = -1;
    kUpgradeSmokeHeight = -1;
    kUpgradeIgnitionHeight = -2;
    kUpgradeRefuelHeight = -3;
    kUpgradeTapeHeight = -4;
    kUpgradeActorsV2Height = -5;
    kUpgradeLiftoffHeight = -6;
    kUpgradeKumquatHeight = -7;
    kUpgradeCalicoHeight = -8;
    kUpgradePersianHeight = -9;
    kUpgradeOrangeHeight = -10;
    kUpgradeClausHeight = -11;
    kUpgradeActorsV3Height = -12;
    kUpgradeNorwegianHeight = -13;
    kUpgradeActorsV4Height = -14;
    kUpgradeHyperdriveHeight = -15;

    kBreezeGasTampingDuration = 0;

    // Update actor constants
    vm::actor::builtin::types::market::kDealUpdatesInterval =
        static_cast<EpochDuration>(kEpochsInDay);

    vm::actor::builtin::types::miner::kWPoStProvingPeriod =
        static_cast<ChainEpoch>(kEpochsInDay);
    vm::actor::builtin::types::miner::kWPoStChallengeWindow =
        static_cast<EpochDuration>(30 * 60 / kEpochDurationSeconds);
    vm::actor::builtin::types::miner::kFaultMaxAge =
        vm::actor::builtin::types::miner::kWPoStProvingPeriod * 14;
    vm::actor::builtin::types::miner::kMinSectorExpiration = 180 * kEpochsInDay;
    vm::actor::builtin::types::miner::kMaxSectorExpirationExtension =
        540 * kEpochsInDay;
    vm::actor::builtin::types::miner::kMaxProveCommitDuration =
        static_cast<EpochDuration>(kEpochsInDay + kPreCommitChallengeDelay);
    std::set<RegisteredSealProof> supportedProofs;
    supportedProofs.insert(RegisteredSealProof::kStackedDrg2KiBV1);
    vm::actor::builtin::types::miner::kSupportedProofs =
        std::move(supportedProofs);

    vm::actor::builtin::types::payment_channel::kSettleDelay =
        kEpochsInHour * 12;

    vm::actor::builtin::types::storage_power::kConsensusMinerMinPower = 2048;

    fc::vm::actor::builtin::types::verified_registry::kMinVerifiedDealSize =
        256;
    kPreCommitChallengeDelay = 10;
  }

  void setParamsNoUpgrades() {
    kEpochDurationSeconds = 4;
    kEpochsInHour = kSecondsInHour / kEpochDurationSeconds;
    kEpochsInDay = 24 * kEpochsInHour;
    kEpochsInYear = 365 * kEpochsInDay;

    kPropagationDelaySecs = 1;

    // Network versions
    kUpgradeBreezeHeight = 100000000;
    kUpgradeSmokeHeight = 100000000;
    kUpgradeIgnitionHeight = 100000000;
    kUpgradeRefuelHeight = 100000000;
    kUpgradeTapeHeight = 100000000;
    kUpgradeActorsV2Height = 100000000;
    kUpgradeLiftoffHeight = 100000000;
    kUpgradeKumquatHeight = 100000000;
    kUpgradeCalicoHeight = 100000000;
    kUpgradePersianHeight = 100000000;
    kUpgradeOrangeHeight = 100000000;
    kUpgradeClausHeight = 100000000;
    kUpgradeActorsV3Height = 100000000;
    kUpgradeNorwegianHeight = 100000000;
    kUpgradeActorsV4Height = 100000000;

    kBreezeGasTampingDuration = 0;

    // Update actor constants
    vm::actor::builtin::types::market::kDealUpdatesInterval =
        static_cast<EpochDuration>(kEpochsInDay);

    vm::actor::builtin::types::miner::kWPoStProvingPeriod =
        static_cast<ChainEpoch>(kEpochsInDay);
    vm::actor::builtin::types::miner::kWPoStChallengeWindow =
        static_cast<EpochDuration>(30 * 60 / kEpochDurationSeconds);
    vm::actor::builtin::types::miner::kFaultMaxAge =
        vm::actor::builtin::types::miner::kWPoStProvingPeriod * 14;
    vm::actor::builtin::types::miner::kMinSectorExpiration = 180 * kEpochsInDay;
    vm::actor::builtin::types::miner::kMaxSectorExpirationExtension =
        540 * kEpochsInDay;
    vm::actor::builtin::types::miner::kMaxProveCommitDuration =
        static_cast<EpochDuration>(kEpochsInDay + kPreCommitChallengeDelay);
    std::set<RegisteredSealProof> supportedProofs;
    supportedProofs.insert(RegisteredSealProof::kStackedDrg2KiBV1);
    vm::actor::builtin::types::miner::kSupportedProofs =
        std::move(supportedProofs);

    vm::actor::builtin::types::payment_channel::kSettleDelay =
        kEpochsInHour * 12;

    vm::actor::builtin::types::storage_power::kConsensusMinerMinPower = 2048;

    fc::vm::actor::builtin::types::verified_registry::kMinVerifiedDealSize =
        256;
  }

  void setParamsInteropnet() {
    // Network versions
    kUpgradeBreezeHeight = -1;
    kUpgradeSmokeHeight = -1;
    kUpgradeIgnitionHeight = -2;
    kUpgradeRefuelHeight = -3;
    kUpgradeTapeHeight = -4;
    kUpgradeActorsV2Height = -5;
    kUpgradeLiftoffHeight = -6;
    kUpgradeKumquatHeight = -7;
    kUpgradeCalicoHeight = -8;
    kUpgradePersianHeight = -9;
    kUpgradeOrangeHeight = -10;
    kUpgradeClausHeight = -11;
    kUpgradeActorsV3Height = -12;
    kUpgradeNorwegianHeight = -13;
    kUpgradeActorsV4Height = -14;
    kUpgradeHyperdriveHeight = -15;

    kBreezeGasTampingDuration = 0;

    // Update actor constants
    vm::actor::builtin::types::miner::kSupportedProofs = {
        RegisteredSealProof::kStackedDrg2KiBV1,
        RegisteredSealProof::kStackedDrg8MiBV1,
        RegisteredSealProof::kStackedDrg512MiBV1,
    };
    vm::actor::builtin::types::storage_power::kConsensusMinerMinPower = 2048;
    fc::vm::actor::builtin::types::verified_registry::kMinVerifiedDealSize =
        256;
    kPreCommitChallengeDelay = 10;
  }
}  // namespace fc
