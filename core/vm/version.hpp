/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_VM_VERSION_HPP
#define FILECOIN_CORE_VM_VERSION_HPP

#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::vm::version {
  using primitives::ChainEpoch;

  /**
   * Enumeration of network upgrades where actor behaviour can change
   */
  enum class NetworkVersion {
    kVersion0,
    kVersion1,
    kVersion2,
    kVersion3,
    kVersion4,
    kVersion5,
    kVersion6,
    kVersion7,
  };

  /**
   * Network version start heights
   */
  const ChainEpoch kUpgradeBreezeHeight = 41280;
  const ChainEpoch kUpgradeSmokeHeight = 51000;
  const ChainEpoch kUpgradeIgnitionHeight = 94000;
  const ChainEpoch kUpgradeRefuelHeight = 130800;
  const ChainEpoch kUpgradeTapeHeight = 140760;
  const ChainEpoch kUpgradeLiftoffHeight = 148888;
  const ChainEpoch kUpgradeKumquatHeight = 170000;

  /**
   * Returns network version for blockchain height
   * @param height - blockchain height
   * @return network version
   */
  inline NetworkVersion getNetworkVersion(const ChainEpoch &height) {
    if (height >= kUpgradeKumquatHeight) return NetworkVersion::kVersion7;
    if (height >= kUpgradeLiftoffHeight) return NetworkVersion::kVersion6;
    if (height >= kUpgradeTapeHeight) return NetworkVersion::kVersion5;
    if (height >= kUpgradeRefuelHeight) return NetworkVersion::kVersion4;
    if (height >= kUpgradeIgnitionHeight) return NetworkVersion::kVersion3;
    if (height >= kUpgradeSmokeHeight) return NetworkVersion::kVersion2;
    if (height >= kUpgradeBreezeHeight) return NetworkVersion::kVersion1;
    return NetworkVersion::kVersion0;
  }
}  // namespace fc::vm::version

#endif  // FILECOIN_CORE_VM_VERSION_HPP
