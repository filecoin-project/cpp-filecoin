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

  DEFINE(kFakeWinningPost){};

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
  DEFINE(kUpgradeAssemblyHeight){138720};
  DEFINE(kUpgradeTapeHeight){140760};
  DEFINE(kUpgradeLiftoffHeight){148888};
  DEFINE(kUpgradeKumquatHeight){170000};
  DEFINE(kUpgradeCalicoHeight){265200};
  DEFINE(kUpgradePersianHeight){272400};
  DEFINE(kUpgradeOrangeHeight){336458};
  // 2020-12-22T02:00:00Z
  DEFINE(kUpgradeClausHeight){343200};
  // 2021-03-04T00:00:30Z
  DEFINE(kUpgradeTrustHeight){550321};
  // 2021-04-12T22:00:00Z
  DEFINE(kUpgradeNorwegianHeight){665280};
  // 2021-04-29T06:00:00Z
  DEFINE(kUpgradeTurboHeight){712320};
  // 2021-06-30T22:00:00Z
  DEFINE(kUpgradeHyperdriveHeight){892800};
  // 2021-10-26T13:30:00Z
  DEFINE(kUpgradeChocolateHeight){1231620};
  // Not applied yet
  DEFINE(kUpgradeOhSnapHeight){INT64_MAX};

  DEFINE(kBreezeGasTampingDuration){120};

  DEFINE(kInteractivePoRepConfidence){6};
}  // namespace fc

namespace fc::vm::actor::builtin::types::market {
  DEFINE(kDealUpdatesInterval) = static_cast<EpochDuration>(kEpochsInDay);
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
    kUpgradeAssemblyHeight = -5;
    kUpgradeLiftoffHeight = -6;
    kUpgradeKumquatHeight = -7;
    // order according to lotus build/params_2k.go
    kUpgradeCalicoHeight = -9;
    kUpgradePersianHeight = -10;
    kUpgradeOrangeHeight = -11;
    kUpgradeClausHeight = -12;
    kUpgradeTrustHeight = -13;
    kUpgradeNorwegianHeight = -14;
    kUpgradeTurboHeight = -15;
    kUpgradeHyperdriveHeight = -16;
    kUpgradeChocolateHeight = -17;
    kUpgradeOhSnapHeight = -18;

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
    kUpgradeAssemblyHeight = 100000000;
    kUpgradeLiftoffHeight = 100000000;
    kUpgradeKumquatHeight = 100000000;
    kUpgradeCalicoHeight = 100000000;
    kUpgradePersianHeight = 100000000;
    kUpgradeOrangeHeight = 100000000;
    kUpgradeClausHeight = 100000000;
    kUpgradeTrustHeight = 100000000;
    kUpgradeNorwegianHeight = 100000000;
    kUpgradeTurboHeight = 100000000;
    kUpgradeChocolateHeight = 100000000;
    kUpgradeOhSnapHeight = 100000000;

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
    kUpgradeAssemblyHeight = -5;
    kUpgradeLiftoffHeight = -6;
    kUpgradeKumquatHeight = -7;
    // order according to lotus build/params_interop.go
    kUpgradeCalicoHeight = -9;
    kUpgradePersianHeight = -10;
    kUpgradeOrangeHeight = -11;
    kUpgradeClausHeight = -12;
    kUpgradeTrustHeight = -13;
    kUpgradeNorwegianHeight = -14;
    kUpgradeTurboHeight = -15;
    kUpgradeHyperdriveHeight = -16;
    kUpgradeChocolateHeight = -17;
    kUpgradeOhSnapHeight = -18;

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
