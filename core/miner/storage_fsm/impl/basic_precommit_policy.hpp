/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/precommit_policy.hpp"

#include "api/full_node/node_api.hpp"

namespace fc::mining {
  using api::FullNodeApi;

  class BasicPreCommitPolicy : public PreCommitPolicy {
   public:
    BasicPreCommitPolicy(std::shared_ptr<FullNodeApi> api,
                         ChainEpoch proving_boundary,
                         ChainEpoch duration);

    outcome::result<ChainEpoch> expiration(
        gsl::span<const types::Piece> pieces) override;

   private:
    std::shared_ptr<FullNodeApi> api_;

    ChainEpoch proving_boundary_;
    ChainEpoch duration_;

    common::Logger logger_;
  };

}  // namespace fc::mining
