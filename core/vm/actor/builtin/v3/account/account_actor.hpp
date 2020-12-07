/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V3_ACCOUNT_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V3_ACCOUNT_ACTOR_HPP

#include "vm/actor/builtin/v2/account/account_actor.hpp"

namespace fc::vm::actor::builtin::v3::account {

  /**
   * Account actor v3 is identical to Account actor v2
   */
  using AccountActorState = v2::account::AccountActorState;
  using Construct = v2::account::Construct;
  using PubkeyAddress = v2::account::PubkeyAddress;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v3::account

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_V3_ACCOUNT_ACTOR_HPP
