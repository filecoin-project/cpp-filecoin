/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace fc::primitives {

  /**
   * @brief epoch index type represents a round of a blockchain protocol, which
   * acts as a proxy for time within the VM
   */
  using ChainEpoch = int64_t;

  using EpochDuration = int64_t;

  constexpr ChainEpoch kChainEpochUndefined{-1};

}  // namespace fc::primitives
