/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_ACTOR_STATE_HANDLE_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_ACTOR_STATE_HANDLE_IMPL_HPP

#include "vm/runtime/actor_state_handle.hpp"

namespace fc::vm::runtime {

  /**
   * @brief Actor state lock implementation
   */
  // TODO(a.chernyshov) implement
  class ActorStateHandleImpl : public ActorStateHandle {
   public:
    ~ActorStateHandleImpl() override = default;
    void updateRelease(ActorSubstateCID new_state_cid) override;
    void release(ActorSubstateCID check_state_cid) override;
    ActorSubstateCID take() override;

   private:
    ActorSubstateCID actor_substate_cid_;
  };

}  // namespace fc::vm::runtime

#endif  // CPP_FILECOIN_CORE_VM_RUNTIME_IMPL_ACTOR_STATE_HANDLE_IMPL_HPP
