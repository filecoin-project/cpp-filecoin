/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PRIMITIVES_CHAIN_EPOCH_HPP
#define CPP_FILECOIN_PRIMITIVES_CHAIN_EPOCH_HPP

#include "primitives/big_int.hpp"

namespace fc::primitives {

  /**
   * @brief epoch index type represents a round of a blockchain protocol, which
   * acts as a proxy for time within the VM
   */
  using ChainEpoch = UBigInt;

  using EpochDuration = BigInt;

}  // namespace fc::primitives

#endif  // CPP_FILECOIN_PRIMITIVES_CHAIN_EPOCH_HPP
