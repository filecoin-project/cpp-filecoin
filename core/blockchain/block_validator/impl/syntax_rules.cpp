/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_validator/impl/syntax_rules.hpp"

namespace fc::blockchain::block_validator {
  outcome::result<void> SyntaxRules::parentsCount(const BlockHeader &block) {
    if (block.height > 0) {
      if ((block.parents.size() > 0) && (block.parents.size() < 5)) {
        return outcome::success();
      }
    } else {
      if (block.parents.empty()) {
        return outcome::success();
      }
    }
    return SyntaxError::INVALID_PARENTS_COUNT;
  }

  outcome::result<void> SyntaxRules::parentWeight(const BlockHeader &block) {
    if (block.parent_weight >= 0) {
      return outcome::success();
    }
    return SyntaxError::INVALID_PARENT_WEIGHT;
  }

  outcome::result<void> SyntaxRules::minerAddress(const BlockHeader &block) {
    if (block.miner.getId() > 0) {
      return outcome::success();
    }
    return SyntaxError::INVALID_MINER_ADDRESS;
  }

  outcome::result<void> SyntaxRules::timestamp(const BlockHeader &block) {
    if (block.timestamp > 0) {
      return outcome::success();
    }
    return SyntaxError::INVALID_TIMESTAMP;
  }

  outcome::result<void> SyntaxRules::ticket(const BlockHeader &block) {
    if (block.ticket) {
      return outcome::success();
    }
    return SyntaxError::INVALID_TICKET;
  }

  outcome::result<void> SyntaxRules::electionPost(const BlockHeader &block) {
    return outcome::success();
  }

  outcome::result<void> SyntaxRules::forkSignal(const BlockHeader &block) {
    if (block.fork_signaling != 0) {
      return outcome::success();
    }
    return SyntaxError::INVALID_FORK_SIGNAL;
  }
}  // namespace fc::blockchain::block_validator

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::block_validator, SyntaxError, e) {
  using fc::blockchain::block_validator::SyntaxError;
  switch (e) {
    case SyntaxError::INVALID_PARENTS_COUNT:
      return "Syntax block validator: invalid Parents count";
    case SyntaxError::INVALID_PARENT_WEIGHT:
      return "Syntax block validator: invalid Parent weight";
    case SyntaxError::INVALID_MINER_ADDRESS:
      return "Syntax block validator: invalid Miner address";
    case SyntaxError::INVALID_TIMESTAMP:
      return "Syntax block validator: invalid Timestamp";
    case SyntaxError::INVALID_TICKET:
      return "Syntax block validator: invalid Ticket";
    case SyntaxError::INVALID_ELECTION_POST:
      return "Syntax block validator: invalid Election PoSt";
    case SyntaxError::INVALID_FORK_SIGNAL:
      return "Syntax block validator: invalid fork signal";
  }
}
