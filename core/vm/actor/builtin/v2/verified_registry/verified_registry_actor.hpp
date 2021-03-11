/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor.hpp"

namespace fc::vm::actor::builtin::v2::verified_registry {

  using Construct = v0::verified_registry::Construct;

  struct AddVerifier : ActorMethodBase<2> {
    using Params = v0::verified_registry::AddVerifier::Params;
    ACTOR_METHOD_DECL();
  };

  struct RemoveVerifier : ActorMethodBase<3> {
    using Params = v0::verified_registry::RemoveVerifier::Params;
    ACTOR_METHOD_DECL();
  };

  struct AddVerifiedClient : ActorMethodBase<4> {
    using Params = v0::verified_registry::AddVerifiedClient::Params;
    ACTOR_METHOD_DECL();
  };

  struct UseBytes : ActorMethodBase<5> {
    using Params = v0::verified_registry::UseBytes::Params;
    ACTOR_METHOD_DECL();
  };

  struct RestoreBytes : ActorMethodBase<6> {
    using Params = v0::verified_registry::RestoreBytes::Params;
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v2::verified_registry
