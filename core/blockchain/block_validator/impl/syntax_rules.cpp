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
    return SyntaxError::kInvalidParentsCount;
  }

  outcome::result<void> SyntaxRules::parentWeight(const BlockHeader &block) {
    if (block.parent_weight >= 0) {
      return outcome::success();
    }
    return SyntaxError::kInvalidParentWeight;
  }

  outcome::result<void> SyntaxRules::minerAddress(const BlockHeader &block) {
    if (block.miner.getId() > 0) {
      return outcome::success();
    }
    return SyntaxError::kInvalidMinerAddress;
  }

  outcome::result<void> SyntaxRules::timestamp(const BlockHeader &block) {
    if (block.timestamp > 0) {
      return outcome::success();
    }
    return SyntaxError::kInvalidTimestamp;
  }

  outcome::result<void> SyntaxRules::ticket(const BlockHeader &block) {
    if (block.ticket) {
      return outcome::success();
    }
    return SyntaxError::kInvalidTicket;
  }

  outcome::result<void> SyntaxRules::electionPost(const BlockHeader &block) {
    return outcome::success();
  }

  outcome::result<void> SyntaxRules::forkSignal(const BlockHeader &block) {
    if (block.fork_signaling != 0) {
      return outcome::success();
    }
    return SyntaxError::kInvalidForkSignal;
  }
}  // namespace fc::blockchain::block_validator

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::block_validator, SyntaxError, e) {
  using fc::blockchain::block_validator::SyntaxError;
  switch (e) {
    case SyntaxError::kInvalidParentsCount:
      return "Syntax block validator: invalid Parents count";
    case SyntaxError::kInvalidParentWeight:
      return "Syntax block validator: invalid Parent weight";
    case SyntaxError::kInvalidMinerAddress:
      return "Syntax block validator: invalid Miner address";
    case SyntaxError::kInvalidTimestamp:
      return "Syntax block validator: invalid Timestamp";
    case SyntaxError::kInvalidTicket:
      return "Syntax block validator: invalid Ticket";
    case SyntaxError::kInvalidElectionPost:
      return "Syntax block validator: invalid Election PoSt";
    case SyntaxError::kInvalidForkSignal:
      return "Syntax block validator: invalid fork signal";
  }
}
