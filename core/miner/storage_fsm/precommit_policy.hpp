/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/types.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"

namespace fc::mining {
  using primitives::ChainEpoch;

  class PreCommitPolicy {
   public:
    virtual ~PreCommitPolicy() = default;

    virtual outcome::result<ChainEpoch> expiration(gsl::span<const types::Piece> pieces) = 0;
  };

}  // namespace fc::mining
