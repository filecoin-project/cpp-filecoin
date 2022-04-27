/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::account {

  // These methods must be actual with the last version of actors

  enum class AccountActor : MethodNumber {
    kConstruct = 1,
    kPubkeyAddress,
  }

  struct Construct : ActorMethodBase<AccountActor::kConstruct> {
    using Params = Address;
  };

  struct PubkeyAddress : ActorMethodBase<AccountActor::kPubkeyAddress> {
    using Result = Address;
  };

}  // namespace fc::vm::actor::builtin::account
