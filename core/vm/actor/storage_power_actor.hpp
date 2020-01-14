/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_HPP

namespace fc::vm::actor {

  enum SpaMethods {
    CONSTRUCTOR = 1,
    CREATE_STORAGE_MINER,
    ARBITRATE_CONSENSUS_FAULT,
    UPDATE_STORAGE,
    GET_TOTAL_STORAGE,
    POWER_LOOKUP,
    IS_VALID_MINER,
    PLEDGE_COLLATERAL_FOR_SIZE,
    CHECK_PROOF_SUBMISSIONS
  };

}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_STORAGE_POWER_ACTOR_HPP
