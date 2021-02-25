/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/reward/policy.hpp"

namespace fc::vm::actor::builtin::v2::reward {
  using primitives::StoragePower;

  /// 330M for testnet
  using v0::reward::kSimpleTotal;
  /// 770M for testnet
  using v0::reward::kBaselineTotal;

  /// 2.5057116798121726 EiB
  static const StoragePower kBaselineInitialValueV2{"2888888880000000000"};

  using v0::reward::kBaselineExponentV3;

  /// PenaltyMultiplier is the factor miner penalties are scaled up by
  constexpr uint kPenaltyMultiplier = 3;

}  // namespace fc::vm::actor::builtin::v2::reward
