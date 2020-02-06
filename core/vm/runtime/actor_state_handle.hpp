/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_ACTOR_STATE_HANDLE_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_ACTOR_STATE_HANDLE_HPP

#include "vm/actor/actor.hpp"

namespace fc::vm::runtime {

  using actor::ActorSubstateCID;

  /**
   * Provides a handle for the actor's state object.
   */
  class ActorStateHandle {
   public:
    virtual ~ActorStateHandle() = default;
    virtual void updateRelease(ActorSubstateCID new_state_cid) = 0;
    virtual void release(ActorSubstateCID check_state_cid) = 0;
    virtual ActorSubstateCID take() = 0;
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_ACTOR_STATE_HANDLE_HPP
