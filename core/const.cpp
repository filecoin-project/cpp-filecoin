/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "const.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/payment_channel/policy.hpp"
#include "vm/actor/builtin/types/storage_power/policy.hpp"
#include "vm/actor/builtin/types/verified_registry/policy.hpp"

#define DEFINE(x) \
  decltype(x) x  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

namespace fc {
  using primitives::sector::RegisteredSealProof;
  using namespace vm::actor::builtin::types;

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
  DEFINE(kDealUpdatesInterval) = kEpochsInDay;
}

namespace fc::vm::actor::builtin::types::miner {
  DEFINE(kWPoStProvingPeriod) = kEpochsInDay;
  DEFINE(kWPoStChallengeWindow) = 30 * 60 / kEpochDurationSeconds;
  DEFINE(kMaxPreCommitRandomnessLookback) = kEpochsInDay + kChainFinality;
  DEFINE(kPreCommitChallengeDelay){150};
  DEFINE(kFaultMaxAge){kWPoStProvingPeriod * 14};
  DEFINE(kMinSectorExpiration) = 180 * kEpochsInDay;
  DEFINE(kSupportedProofs){
      RegisteredSealProof::kStackedDrg32GiBV1,
      RegisteredSealProof::kStackedDrg64GiBV1,
  };
  DEFINE(kMaxSectorExpirationExtension) = 540 * kEpochsInDay;
  DEFINE(kMaxProveCommitDuration) = kEpochsInDay + kPreCommitChallengeDelay;
}  // namespace fc::vm::actor::builtin::types::miner

namespace fc::vm::actor::builtin::types::payment_channel {
  DEFINE(kSettleDelay) = kEpochsInHour * 12;
}

namespace fc::vm::actor::builtin::types::storage_power {
  DEFINE(kConsensusMinerMinPower){StoragePower{10} << 40};
}

namespace fc::vm::actor::builtin::types::verified_registry {
  DEFINE(kMinVerifiedDealSize){StoragePower{1} << 20};
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
    market::kDealUpdatesInterval = static_cast<EpochDuration>(kEpochsInDay);

    miner::kWPoStProvingPeriod = kEpochsInDay;
    miner::kWPoStChallengeWindow = 30 * 60 / kEpochDurationSeconds;
    miner::kMaxPreCommitRandomnessLookback =
        kEpochsInDay + miner::kChainFinality;
    miner::kFaultMaxAge = miner::kWPoStProvingPeriod * 14;
    miner::kMinSectorExpiration = 180 * kEpochsInDay;
    miner::kMaxSectorExpirationExtension = 540 * kEpochsInDay;
    miner::kMaxProveCommitDuration =
        kEpochsInDay + miner::kPreCommitChallengeDelay;
    miner::kSupportedProofs = {
        RegisteredSealProof::kStackedDrg2KiBV1,
        RegisteredSealProof::kStackedDrg8MiBV1,
    };

    payment_channel::kSettleDelay = kEpochsInHour * 12;

    storage_power::kConsensusMinerMinPower = 2048;

    verified_registry::kMinVerifiedDealSize = 256;
    miner::kPreCommitChallengeDelay = 10;
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
    market::kDealUpdatesInterval = static_cast<EpochDuration>(kEpochsInDay);

    miner::kWPoStProvingPeriod = kEpochsInDay;
    miner::kWPoStChallengeWindow = 30 * 60 / kEpochDurationSeconds;
    miner::kMaxPreCommitRandomnessLookback =
        kEpochsInDay + miner::kChainFinality;
    miner::kFaultMaxAge = miner::kWPoStProvingPeriod * 14;
    miner::kMinSectorExpiration = 180 * kEpochsInDay;
    miner::kMaxSectorExpirationExtension = 540 * kEpochsInDay;
    miner::kMaxProveCommitDuration =
        kEpochsInDay + miner::kPreCommitChallengeDelay;
    miner::kSupportedProofs = {RegisteredSealProof::kStackedDrg2KiBV1};

    payment_channel::kSettleDelay = kEpochsInHour * 12;

    storage_power::kConsensusMinerMinPower = 2048;

    verified_registry::kMinVerifiedDealSize = 256;
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
    miner::kSupportedProofs = {
        RegisteredSealProof::kStackedDrg2KiBV1,
        RegisteredSealProof::kStackedDrg8MiBV1,
        RegisteredSealProof::kStackedDrg512MiBV1,
    };
    storage_power::kConsensusMinerMinPower = 2048;
    verified_registry::kMinVerifiedDealSize = 256;
    miner::kPreCommitChallengeDelay = 10;
  }
}  // namespace fc
