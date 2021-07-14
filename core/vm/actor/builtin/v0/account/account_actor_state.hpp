/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/account_actor_state.hpp"

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::vm::actor::builtin::v0::account {
  struct AccountActorState : states::AccountActorState {};
  CBOR_TUPLE(AccountActorState, address)
}  // namespace fc::vm::actor::builtin::v0::account
