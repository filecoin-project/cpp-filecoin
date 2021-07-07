/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v3::verified_registry {
  ACTOR_STATE_TO_CBOR_THIS(VerifiedRegistryActorState)
}  // namespace fc::vm::actor::builtin::v3::verified_registry
