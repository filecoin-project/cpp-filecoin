/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CHAIN_VALIDATOR_SYNTAX_RULES_HPP
#define CPP_FILECOIN_CORE_CHAIN_VALIDATOR_SYNTAX_RULES_HPP

#include "common/outcome.hpp"
#include "primitives/block/block.hpp"

namespace fc::blockchain::block_validator {
  /**
   * @class Validating block header syntax
   */
  class SyntaxRules {
   protected:
    using BlockHeader = primitives::block::BlockHeader;

   public:
    /**
     * @brief Check parents count
     * @param header - block to check
     * @return check result
     */
    static outcome::result<void> parentsCount(const BlockHeader &header);

    /**
     * @brief Check parents weight
     * @param header - block to check
     * @return check result
     */
    static outcome::result<void> parentWeight(const BlockHeader &header);

    /**
     * @brief Check miner address
     * @param header - block to check
     * @return check result
     */
    static outcome::result<void> minerAddress(const BlockHeader &header);

    /**
     * @brief Check timestamp
     * @param header - block to check
     * @return check result
     */
    static outcome::result<void> timestamp(const BlockHeader &header);

    /**
     * @brief Check winning ticket
     * @param header - block to check
     * @return check result
     */
    static outcome::result<void> ticket(const BlockHeader &header);

    /**
     * @brief Check Epost terms
     * @param header - block to check
     * @return check result
     */
    static outcome::result<void> electionPost(const BlockHeader &header);

    /**
     * @brief Check fork signal field
     * @param header - block to check
     * @return check result
     */
    static outcome::result<void> forkSignal(const BlockHeader &header);
  };

  enum class SyntaxError {
    INVALID_PARENTS_COUNT = 1,
    INVALID_PARENT_WEIGHT,
    INVALID_MINER_ADDRESS,
    INVALID_TIMESTAMP,
    INVALID_TICKET,
    INVALID_ELECTION_POST,
    INVALID_FORK_SIGNAL,
  };
}  // namespace fc::blockchain::block_validator

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::block_validator, SyntaxError);

#endif
