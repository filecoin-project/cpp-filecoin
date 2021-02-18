/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/states/account_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::account {
  struct AccountActorState : states::AccountActorState {
    AccountActorState() : states::AccountActorState(ActorVersion::kVersion2) {}
  };
  CBOR_TUPLE(AccountActorState, address)
}  // namespace fc::vm::actor::builtin::v2::account
