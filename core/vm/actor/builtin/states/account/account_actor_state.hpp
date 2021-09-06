/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/address/address.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::address::Address;

  struct AccountActorState {
    virtual ~AccountActorState() = default;

    Address address;
  };

  using AccountActorStatePtr = types::Universal<AccountActorState>;

}  // namespace fc::vm::actor::builtin::states
