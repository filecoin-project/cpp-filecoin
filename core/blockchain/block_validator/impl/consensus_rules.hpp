/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "blockchain/weight_calculator.hpp"
#include "common/outcome.hpp"
#include "power/power_table.hpp"
#include "primitives/block/block.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::blockchain::block_validator {
  class ConsensusRules {
   protected:
    using WeightCalculator = weight::WeightCalculator;
    using BlockHeader = primitives::block::BlockHeader;
    using PowerTable = power::PowerTable;
    using ChainEpoch = primitives::ChainEpoch;
    using Tipset = primitives::tipset::Tipset;

   public:
    /**
     * @brief Check miner params
     * @param header - block to check
     * @param power_table - table with miners power params
     * @return check result
     */
    static outcome::result<void> activeMiner(
        const BlockHeader &header,
        const std::shared_ptr<PowerTable> &power_table);

    /**
     * @brief Check tipset weight
     * @param header - block to check
     * @param parent_tipset
     * @param weight_calculator
     * @return check result
     */
    static outcome::result<void> parentWeight(
        const BlockHeader &header,
        const Tipset &parent_tipset,
        const std::shared_ptr<WeightCalculator> &weight_calculator);

    /**
     * @brief Check block epoch
     * @param header - block to check
     * @param current_epoch
     * @return check result
     */
    static outcome::result<void> epoch(const BlockHeader &header,
                                       ChainEpoch current_epoch);
  };

  enum class ConsensusError {
    kInvalidMiner = 1,
    kGetParentTipsetError,
    kInvalidParentWeight,
    kBlockEpochInFuture,
    kBlockEpochTooFar,
  };
}  // namespace fc::blockchain::block_validator

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::block_validator, ConsensusError);
