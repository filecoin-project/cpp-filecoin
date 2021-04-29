/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "const.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

#define DEFINE(x) decltype(x) x

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

  DEFINE(kBreezeGasTampingDuration){120};

  DEFINE(kInteractivePoRepConfidence){6};
}  // namespace fc

namespace fc::vm::actor::builtin::types::market {
  EpochDuration kDealUpdatesInterval{static_cast<EpochDuration>(kEpochsInDay)};
}

namespace fc::vm::actor::builtin::types::miner {
  ChainEpoch kWPoStProvingPeriod = kEpochsInDay;
  EpochDuration kWPoStChallengeWindow = 30 * 60 / kEpochDurationSeconds;
  EpochDuration kFaultMaxAge{kWPoStProvingPeriod * 14};
  ChainEpoch kMinSectorExpiration = 180 * kEpochsInDay;
  std::set<RegisteredSealProof> kSupportedProofs{
      RegisteredSealProof::kStackedDrg32GiBV1,
      RegisteredSealProof::kStackedDrg64GiBV1,
  };
  ChainEpoch kMaxSectorExpirationExtension = 540 * kEpochsInDay;
  EpochDuration kMaxProveCommitDuration =
      kEpochsInDay + kPreCommitChallengeDelay;
}  // namespace fc::vm::actor::builtin::types::miner

namespace fc::vm::actor::builtin::types::payment_channel {
  EpochDuration kSettleDelay = kEpochsInHour * 12;
}

namespace fc::vm::actor::builtin::types::storage_power {
  StoragePower kConsensusMinerMinPower{StoragePower{10} << 40};
}

namespace fc::vm::actor::builtin::types::verified_registry {
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
    kUpgradeActorsV2Height = 10;
    kUpgradeLiftoffHeight = -5;
    kUpgradeKumquatHeight = 15;
    kUpgradeCalicoHeight = 20;
    kUpgradePersianHeight = 25;
    kUpgradeOrangeHeight = 27;
    kUpgradeClausHeight = 30;
    kUpgradeActorsV3Height = 35;
    kUpgradeNorwegianHeight = 40;
    kUpgradeActorsV4Height = 45;

    kBreezeGasTampingDuration = 0;

    // Update actor constants
    vm::actor::builtin::types::market::kDealUpdatesInterval =
        static_cast<EpochDuration>(kEpochsInDay);

    vm::actor::builtin::types::miner::kWPoStProvingPeriod = kEpochsInDay;
    vm::actor::builtin::types::miner::kWPoStChallengeWindow =
        30 * 60 / kEpochDurationSeconds;
    vm::actor::builtin::types::miner::kFaultMaxAge =
        vm::actor::builtin::types::miner::kWPoStProvingPeriod * 14;
    vm::actor::builtin::types::miner::kMinSectorExpiration = 180 * kEpochsInDay;
    vm::actor::builtin::types::miner::kMaxSectorExpirationExtension =
        540 * kEpochsInDay;
    vm::actor::builtin::types::miner::kMaxProveCommitDuration =
        kEpochsInDay + kPreCommitChallengeDelay;
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

    vm::actor::builtin::types::miner::kWPoStProvingPeriod = kEpochsInDay;
    vm::actor::builtin::types::miner::kWPoStChallengeWindow =
        30 * 60 / kEpochDurationSeconds;
    vm::actor::builtin::types::miner::kFaultMaxAge =
        vm::actor::builtin::types::miner::kWPoStProvingPeriod * 14;
    vm::actor::builtin::types::miner::kMinSectorExpiration = 180 * kEpochsInDay;
    vm::actor::builtin::types::miner::kMaxSectorExpirationExtension =
        540 * kEpochsInDay;
    vm::actor::builtin::types::miner::kMaxProveCommitDuration =
        kEpochsInDay + kPreCommitChallengeDelay;
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
}  // namespace fc
