/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CHAIN_BLOCK_VALIDATOR_SCENARIOS_HPP
#define CPP_FILECOIN_CORE_CHAIN_BLOCK_VALIDATOR_SCENARIOS_HPP

namespace fc::blockchain::block_validator::scenarios {
  /**
   * @enum Validation stages
   */
  enum class Stage {
    SYNTAX_BV0,
    CONSENSUS_BV1,
    BLOCK_SIGNATURE_BV2,
    ELECTION_POST_BV3,
    MESSAGE_SIGNATURE_BV4,
    STATE_TREE_BV5
  };

  /**
   * @brief Validation scenariouses
   */
  using Scenario = std::initializer_list<Stage>;

  /**
   * Full validation
   */
  const Scenario kFullValidation{Stage::SYNTAX_BV0,
                                 Stage::CONSENSUS_BV1,
                                 Stage::BLOCK_SIGNATURE_BV2,
                                 Stage::ELECTION_POST_BV3,
                                 Stage::MESSAGE_SIGNATURE_BV4,
                                 Stage::STATE_TREE_BV5};

}  // namespace fc::blockchain::block_validator::scenarios

#endif
