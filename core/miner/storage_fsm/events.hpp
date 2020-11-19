/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_EVENTS_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_EVENTS_HPP

#include "primitives/tipset/tipset.hpp"
#include "vm/actor/builtin/v0/miner/policy.hpp"

namespace fc::mining {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::tipset::Tipset;

  constexpr ChainEpoch kGlobalChainConfidence =
      2 * vm::actor::builtin::v0::miner::kChainFinalityish;

  class Events {
   public:
    /**
     * @param confidence = current_height - tipset.height
     */
    using HeightHandler =
        std::function<outcome::result<void>(const Tipset &, ChainEpoch)>;
    using RevertHandler = std::function<outcome::result<void>(const Tipset &)>;

    virtual ~Events() = default;

    virtual outcome::result<void> subscribeHeadChanges() = 0;

    virtual void unsubscribeHeadChanges() = 0;

    virtual outcome::result<void> chainAt(HeightHandler,
                                          RevertHandler,
                                          EpochDuration confidence,
                                          ChainEpoch height) = 0;
  };

  enum class EventsError {
    kNotFoundTipset = 1,
  };

}  // namespace fc::mining

OUTCOME_HPP_DECLARE_ERROR(fc::mining, EventsError);

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_EVENTS_HPP
