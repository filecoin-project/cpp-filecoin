/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor.hpp"
#include "vm/runtime/actor_context.hpp"

namespace fc::vm::runtime::context {
  using actor::Actor;
  using actor::ActorVersion;
  using actor::CodeId;
  using version::NetworkVersion;

  class ActorContextCreator {
   public:
    static ActorContextPtr createContext(ActorVersion version);
    static ActorContextPtr createContext(const CodeId &actorCid);
    static ActorContextPtr createContext(const Actor &actor);
    static ActorContextPtr createContext(const NetworkVersion &network_version);
  };
}  // namespace fc::vm::runtime::context
