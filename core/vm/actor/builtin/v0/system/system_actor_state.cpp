/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/system/system_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v0::system {
  ACTOR_STATE_TO_CBOR_THIS(SystemActorState)
}  // namespace fc::vm::actor::builtin::v0::system
