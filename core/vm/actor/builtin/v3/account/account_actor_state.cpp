/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/account/account_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v3::account {
  ACTOR_STATE_TO_CBOR_THIS(AccountActorState)
}  // namespace fc::vm::actor::builtin::v3::account
