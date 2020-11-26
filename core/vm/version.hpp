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
  };

  const NetworkVersion kLatestVersion = NetworkVersion::kVersion6;

  /**
   * Network version end heights
   */
  /** UpgradeBreeze, <= v0 network */
  const ChainEpoch kUpgradeBreezeHeight = 41280;

  /** UpgradeSmoke, <= v1 network */
  const ChainEpoch kUpgradeSmokeHeight = 51000;

  /** UpgradeIgnition, <= v2 network */
  const ChainEpoch kUpgradeIgnitionHeight = 94000;

  /** UpgradeRefuel, <= v3 network */
  const ChainEpoch kUpgradeRefuelHeight = 130800;

  /** UpgradeActorsV2, <= v3 network */
  const ChainEpoch kUpgradeActorsV2Height = 138720;

  /** UpgradeTape, <= v4 network */
  const ChainEpoch kUpgradeTapeHeight = 140760;

  /** UpgradeLiftoff, <= v5 network */
  const ChainEpoch kUpgradeLiftoffHeight = 148888;

  /** UpgradeKumquat, <= v5 network */
  const ChainEpoch kUpgradeKumquatHeight = 170000;

  /**
   * Returns network version for blockchain height
   * @param height - blockchain height
   * @return network version
   */
  inline NetworkVersion getNetworkVersion(const ChainEpoch &height) {
    if (height <= kUpgradeBreezeHeight) return NetworkVersion::kVersion0;
    if (height <= kUpgradeSmokeHeight) return NetworkVersion::kVersion1;
    if (height <= kUpgradeIgnitionHeight) return NetworkVersion::kVersion2;
    if (height <= kUpgradeRefuelHeight) return NetworkVersion::kVersion3;
    if (height <= kUpgradeActorsV2Height) return NetworkVersion::kVersion3;
    if (height <= kUpgradeTapeHeight) return NetworkVersion::kVersion4;
    if (height <= kUpgradeLiftoffHeight) return NetworkVersion::kVersion5;
    if (height <= kUpgradeKumquatHeight) return NetworkVersion::kVersion5;
    return kLatestVersion;
  }
}  // namespace fc::vm::version

#endif  // FILECOIN_CORE_VM_VERSION_HPP
