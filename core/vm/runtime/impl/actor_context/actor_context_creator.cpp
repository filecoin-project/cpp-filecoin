/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/actor_context_creator.hpp"

#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v2/codes.hpp"
#include "vm/actor/builtin/v3/codes.hpp"
#include "vm/runtime/impl/actor_context/actor_context_v0.hpp"
#include "vm/runtime/impl/actor_context/actor_context_v2.hpp"
#include "vm/runtime/impl/actor_context/actor_context_v3.hpp"

namespace fc::vm::runtime::context {
  using namespace actor::builtin;

  ActorContextPtr ActorContextCreator::createContext(
      ActorVersion version) {
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<ActorContextV0>();
      case ActorVersion::kVersion2:
        return std::make_shared<ActorContextV2>();
      case ActorVersion::kVersion3:
        return std::make_shared<ActorContextV3>();
    }
  }

  ActorContextPtr ActorContextCreator::createContext(
      const CodeId &actorCid) {
    const auto version = actor::getActorVersionForCid(actorCid);
    return createContext(version);
  }

  ActorContextPtr ActorContextCreator::createContext(const Actor &actor) {
    return createContext(actor.code);
  }

  ActorContextPtr ActorContextCreator::createContext(
      const NetworkVersion &network_version) {
    const auto version = actor::getActorVersionForNetwork(network_version);
    return createContext(version);
  }
}  // namespace fc::vm::runtime::context
