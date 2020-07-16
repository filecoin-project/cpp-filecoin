/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_validator/impl/consensus_rules.hpp"

namespace fc::blockchain::block_validator {
  outcome::result<void> ConsensusRules::activeMiner(
      const BlockHeader &block,
      const std::shared_ptr<PowerTable> &power_table) {
    auto power_response = power_table->getMinerPower(block.miner);
    if (power_response.has_value() && power_response.value() > 0) {
      return outcome::success();
    }
    return ConsensusError::kInvalidMiner;
  }

  outcome::result<void> ConsensusRules::parentWeight(
      const BlockHeader &block,
      const Tipset &parent_tipset,
      const std::shared_ptr<WeightCalculator> &weight_calculator) {
    auto calculated_weight = weight_calculator->calculateWeight(parent_tipset);
    if (calculated_weight.has_value()) {
      if (calculated_weight.value() == block.parent_weight) {
        return outcome::success();
      }
    }
    return ConsensusError::kInvalidParentWeight;
  }

  outcome::result<void> ConsensusRules::epoch(const BlockHeader &block,
                                              ChainEpoch current_epoch) {
    if (block.height > static_cast<uint64_t>(current_epoch)) {
      return ConsensusError::kBlockEpochInFuture;
    }
    // TODO: block epoch mast be not farther in the past than the soft
    // finality as defined by SPC
    return outcome::success();
  }
}  // namespace fc::blockchain::block_validator

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::block_validator,
                            ConsensusError,
                            e) {
  using fc::blockchain::block_validator::ConsensusError;
  switch (e) {
    case ConsensusError::kInvalidMiner:
      return "Block validation: invalid miner";
    case ConsensusError::kGetParentTipsetError:
      return "Block validation: get parent tipset error";
    case ConsensusError::kBlockEpochInFuture:
      return "Block validation: block epoch in future";
    case ConsensusError::kInvalidParentWeight:
      return "Block validation: invalid parent weight";
    case ConsensusError::kBlockEpochTooFar:
      return "Block validation: block eposch too far";
  }
  return "Block validation: unknown error";
}
