/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V2_ACCOUNT_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V2_ACCOUNT_ACTOR_HPP

#include "vm/actor/builtin/v0/account/account_actor.hpp"

namespace fc::vm::actor::builtin::v2::account {

  /**
   * Account actor v2 is identical to Account actor v0
   */
  using AccountActorState = v0::account::AccountActorState;
  using Construct = v0::account::Construct;
  using PubkeyAddress = v0::account::PubkeyAddress;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v2::account

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V2_ACCOUNT_ACTOR_HPP
