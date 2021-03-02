/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/reward/reward_actor.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  using fc::primitives::GasAmount;
  using fc::primitives::SectorStorageWeightDesc;
  using fc::primitives::StoragePower;
  using fc::primitives::TokenAmount;

  /**
   * Minimum power of an individual miner to meet the threshold for leader
   * election.
   * 1 << 40
   * MAINNET: 10 << 40
   */
  inline const StoragePower kConsensusMinerMinPower{StoragePower{10} << 40};
  constexpr size_t kSectorQualityPrecision{20};

  /**
   * Maximum number of prove commits a miner can submit in one epoch
   */
  constexpr size_t kMaxMinerProveCommitsPerEpoch{200};

  /**
   * Amount of gas charged for SubmitPoRepForBulkVerify. This number is
   * empirically determined
   */
  static constexpr GasAmount kGasOnSubmitVerifySeal{34721049};

  StoragePower qaPowerForWeight(const SectorStorageWeightDesc &weight);

  inline TokenAmount initialPledgeForWeight(
      const StoragePower &qa,
      const StoragePower &total_qa,
      const TokenAmount &circ_supply,
      const TokenAmount &total_pledge,
      const TokenAmount &per_epoch_reward) {
    return bigdiv(qa * per_epoch_reward, total_qa);
  }
}  // namespace fc::vm::actor::builtin::v0::storage_power
