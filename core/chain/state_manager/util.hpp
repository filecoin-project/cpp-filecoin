/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CHAIN_STATE_MANAGER_UTIL_HPP
#define CPP_FILECOIN_CORE_CHAIN_STATE_MANAGER_UTIL_HPP

#include "chain/state_manager/state_manager.hpp"
#include "common/outcome.hpp"
#include "primitives/address/address.hpp"

namespace fc::chain::stmgr::util {
  outcome::result<primitives::address::Address> getMinerWorkerRaw(
      std::shared_ptr<StateManager> state_manager,
      const CID &cid,
      const primitives::address::Address &miner_address) {
    primitives::chain::Message{miner_address, miner_address,  ;
      OUTCOME_TRY(receipt, state_manager->callRaw())
  }
}  // namespace fc::chain::stmgr::util

#endif  // CPP_FILECOIN_CORE_CHAIN_STATE_MANAGER_UTIL_HPP
