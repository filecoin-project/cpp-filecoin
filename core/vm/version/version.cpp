/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/version/version.hpp"
#include "const.hpp"

namespace fc::vm::version {

  /**
   * Returns network version for blockchain height
   * @param height - blockchain height
   * @return network version
   */
  NetworkVersion getNetworkVersion(const ChainEpoch &height) {
    if (height <= kUpgradeBreezeHeight) return NetworkVersion::kVersion0;
    if (height <= kUpgradeSmokeHeight) return NetworkVersion::kVersion1;
    if (height <= kUpgradeIgnitionHeight) return NetworkVersion::kVersion2;
    if (height <= kUpgradeRefuelHeight) return NetworkVersion::kVersion3;
    if (height <= kUpgradeAssemblyHeight) return NetworkVersion::kVersion3;
    if (height <= kUpgradeTapeHeight) return NetworkVersion::kVersion4;
    if (height <= kUpgradeLiftoffHeight) return NetworkVersion::kVersion5;
    if (height <= kUpgradeKumquatHeight) return NetworkVersion::kVersion5;
    if (height <= kUpgradeCalicoHeight) return NetworkVersion::kVersion6;
    if (height <= kUpgradePersianHeight) return NetworkVersion::kVersion7;
    if (height <= kUpgradeOrangeHeight) return NetworkVersion::kVersion8;
    if (height <= kUpgradeTrustHeight) return NetworkVersion::kVersion9;
    if (height <= kUpgradeNorwegianHeight) return NetworkVersion::kVersion10;
    if (height <= kUpgradeTurboHeight) return NetworkVersion::kVersion11;
    if (height <= kUpgradeHyperdriveHeight) return NetworkVersion::kVersion12;
    if (height <= kUpgradeChocolateHeight) return NetworkVersion::kVersion13;
    if (height <= kUpgradeOhSnapHeight) return NetworkVersion::kVersion14;
    return kLatestVersion;
  }
}  // namespace fc::vm::version
