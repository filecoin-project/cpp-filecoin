/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::storage_power {
  using primitives::BigInt;
  using primitives::GasAmount;
  using primitives::SectorStorageWeightDesc;
  using primitives::StoragePower;
  using primitives::TokenAmount;

  /**
   * Minimum power of an individual miner to meet the threshold for leader
   * election.
   */
  extern StoragePower kConsensusMinerMinPower;

  /**
   * Maximum number of prove commits a miner can submit in one epoch
   */
  constexpr size_t kMaxMinerProveCommitsPerEpoch{200};

  /**
   * Amount of gas charged for SubmitPoRepForBulkVerify. This number is
   * empirically determined
   */
  static constexpr GasAmount kGasOnSubmitVerifySeal{34721049};

  inline TokenAmount initialPledgeForWeight(
      const StoragePower &qa,
      const StoragePower &total_qa,
      const TokenAmount &circ_supply,
      const TokenAmount &total_pledge,
      const TokenAmount &per_epoch_reward) {
    return bigdiv(qa * per_epoch_reward, total_qa);
  }
}  // namespace fc::vm::actor::builtin::types::storage_power
