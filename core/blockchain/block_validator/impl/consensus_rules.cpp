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
    return ConsensusError::INVALID_MINER;
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
    return ConsensusError::INVALID_PARENT_WEIGHT;
  }

  outcome::result<void> ConsensusRules::epoch(const BlockHeader &block,
                                              ChainEpoch current_epoch) {
    if (block.height > static_cast<uint64_t>(current_epoch)) {
      return ConsensusError::BLOCK_EPOCH_IN_FUTURE;
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
    case ConsensusError::INVALID_MINER:
      return "Block validation: invalid miner";
    case ConsensusError::GET_PARENT_TIPSET_ERROR:
      return "Block validation: get parent tipset error";
    case ConsensusError::BLOCK_EPOCH_IN_FUTURE:
      return "Block validation: block epoch in future";
    case ConsensusError::INVALID_PARENT_WEIGHT:
      return "Block validation: invalid parent weight";
    case ConsensusError::BLOCK_EPOCH_TOO_FAR:
      return "Block validation: block eposch too far";
  }
  return "Block validation: unknown error";
}
