/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/tipset/tipset.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::mining {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::tipset::TipsetCPtr;

  constexpr ChainEpoch kGlobalChainConfidence =
      2 * vm::actor::builtin::types::miner::kChainFinality;

  class Events {
   public:
    /**
     * @param confidence = current_height - tipset.height
     */
    using HeightHandler =
        std::function<outcome::result<void>(TipsetCPtr, ChainEpoch)>;
    using RevertHandler = std::function<outcome::result<void>(TipsetCPtr)>;

    virtual ~Events() = default;

    virtual outcome::result<void> chainAt(HeightHandler,
                                          RevertHandler,
                                          EpochDuration confidence,
                                          ChainEpoch height) = 0;
  };

}  // namespace fc::mining
