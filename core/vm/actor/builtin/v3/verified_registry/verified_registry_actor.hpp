/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v2/verified_registry/verified_registry_actor.hpp"

namespace fc::vm::actor::builtin::v3::verified_registry {

  using Construct = v2::verified_registry::Construct;
  using AddVerifier = v2::verified_registry::AddVerifier;
  using RemoveVerifier = v2::verified_registry::RemoveVerifier;
  using AddVerifiedClient = v2::verified_registry::AddVerifiedClient;
  using RestoreBytes = v2::verified_registry::RestoreBytes;
  using UseBytes = v2::verified_registry::UseBytes;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v3::verified_registry
