/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/actor_state_handle_impl.hpp"

using fc::vm::actor::ActorSubstateCID;
using fc::vm::runtime::ActorStateHandleImpl;


void ActorStateHandleImpl::updateRelease(ActorSubstateCID new_state_cid) {
  actor_substate_cid_ = new_state_cid;
}

void ActorStateHandleImpl::release(ActorSubstateCID check_state_cid) {
  actor_substate_cid_ = check_state_cid;
}

ActorSubstateCID ActorStateHandleImpl::take() {
  return actor_substate_cid_;
}
