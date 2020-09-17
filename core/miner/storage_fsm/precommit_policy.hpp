/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_PRECOMMIT_POLICY_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_PRECOMMIT_POLICY_HPP

#include "miner/storage_fsm/types.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::mining {
  using primitives::ChainEpoch;

  class PreCommitPolicy {
   public:
    virtual ~PreCommitPolicy() = default;

    virtual ChainEpoch expiration(gsl::span<const types::Piece> pieces) = 0;
  };

}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_PRECOMMIT_POLICY_HPP
