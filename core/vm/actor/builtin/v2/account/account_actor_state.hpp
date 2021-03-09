/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/builtin/states/account_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::account {
  struct AccountActorState : states::AccountActorState {
    outcome::result<Buffer> toCbor() const override;
  };
  CBOR_TUPLE(AccountActorState, address)
}  // namespace fc::vm::actor::builtin::v2::account
