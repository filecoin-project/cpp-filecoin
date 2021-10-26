/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::vm::version {
  using primitives::ChainEpoch;

  /**
   * Enumeration of network upgrades where actor behaviour can change
   */
  enum class NetworkVersion : int64_t {
    kVersion0,
    kVersion1,
    kVersion2,
    kVersion3,
    kVersion4,
    kVersion5,
    kVersion6,
    kVersion7,
    kVersion8,
    kVersion9,
    kVersion10,
    kVersion11,
    kVersion12,
    kVersion13,
    kVersion14,
  };

  constexpr auto kLatestVersion{NetworkVersion::kVersion14};

  /**
   * Returns network version for blockchain height
   * @param height - blockchain height
   * @return network version
   */
  NetworkVersion getNetworkVersion(const ChainEpoch &height);
}  // namespace fc::vm::version
