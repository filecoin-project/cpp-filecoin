/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SEALING_IMPL_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SEALING_IMPL_HPP

#include <common/logger.hpp>
#include "miner/storage_fsm/sealing.hpp"

#include "fsm/fsm.hpp"
#include "miner/storage_fsm/sealing_states.hpp"
#include "miner/storage_fsm/selaing_events.hpp"

namespace fc::mining {
  using SealingTransition = fsm::Transition<SealingEvent, SealingState>;

  class SealingImpl : public Sealing {
   private:
    /**
     * Creates all FSM transitions
     * @return vector of transitions for fsm
     */
    std::vector<SealingTransition> makeFSMTransitions();

    common::Logger logger_;
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_IMPL_SEALING_IMPL_HPP
