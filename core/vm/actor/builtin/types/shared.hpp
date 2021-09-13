/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * Code shared by multiple built-in actors
 */

namespace fc::vm::actor::builtin::types {

  /// Quality multiplier for committed capacity (no deals) in a sector
  constexpr uint64_t kQualityBaseMultiplier{10};

  /// Quality multiplier for unverified deals in a sector
  constexpr uint64_t kDealWeightMultiplier{10};

  /// Quality multiplier for verified deals in a sector
  constexpr uint64_t kVerifiedDealWeightMultiplier{100};

  /// Precision used for making QA power calculations
  constexpr uint64_t kSectorQualityPrecision{20};

}  // namespace fc::vm::actor::builtin::types
