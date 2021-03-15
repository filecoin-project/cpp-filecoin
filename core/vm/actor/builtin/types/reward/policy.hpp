/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::reward {
  using primitives::BigInt;
  using primitives::StoragePower;
  using primitives::TokenAmount;

  /// 330M for testnet
  static const TokenAmount kSimpleTotal = BigInt(330e6) * BigInt(1e18);
  /// 770M for testnet
  static const TokenAmount kBaselineTotal = BigInt(770e6) * BigInt(1e18);

  /// 1EiB
  static const StoragePower kBaselineInitialValueV0 = BigInt(1) << 60;

  /// 2.5057116798121726 EiB
  static const StoragePower kBaselineInitialValueV2{"2888888880000000000"};

  /**
   * 36.266260308195979333 FIL
   * https://www.wolframalpha.com/input/?i=IntegerPart%5B330%2C000%2C000+*+%281+-+Exp%5B-Log%5B2%5D+%2F+%286+*+%281+year+%2F+30+seconds%29%29%5D%29+*+10%5E18%5D
   */
  static const BigInt kInitialRewardPositionEstimate{"36266260308195979333"};

  /**
   * -1.0982489*10^-7 FIL per epoch.
   * Change of simple minted tokens between epochs 0 and 1
   * https://www.wolframalpha.com/input/?i=IntegerPart%5B%28Exp%5B-Log%5B2%5D+%2F+%286+*+%281+year+%2F+30+seconds%29%29%5D+-+1%29+*+10%5E18%5D
   */
  static const BigInt kInitialRewardVelocityEstimate{-109897758509};

  /**
   * Baseline exponent for network version 0
   * Floor(e^(ln[1 + 200%] / epochsInYear) * 2^128
   * Q.128 formatted number such that f(epoch) = baseExponent^epoch grows 200%
   * in one year of epochs Calculation here:
   * https://www.wolframalpha.com/input/?i=IntegerPart%5BExp%5BLog%5B1%2B200%25%5D%2F%28%28365+days%29%2F%2830+seconds%29%29%5D*2%5E128%5D
   */
  static const BigInt kBaselineExponentV0{
      "340282722551251692435795578557183609728"};

  /**
   * Baseline exponent for network version 3
   * Floor(e^(ln[1 + 100%] / epochsInYear) * 2^128
   * Q.128 formatted number such that f(epoch) = baseExponent^epoch grows 100%
   * in one year of epochs
   * Calculation here:
   * https://www.wolframalpha.com/input/?i=IntegerPart%5BExp%5BLog%5B1%2B100%25%5D%2F%28%28365+days%29%2F%2830+seconds%29%29%5D*2%5E128%5D
   */
  static const BigInt kBaselineExponentV3{
      "340282591298641078465964189926313473653"};

  /// PenaltyMultiplier is the factor miner penalties are scaled up by
  constexpr uint kPenaltyMultiplier = 3;

}  // namespace fc::vm::actor::builtin::types::reward
