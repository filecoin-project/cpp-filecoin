/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor.hpp"

#include "vm/actor/builtin/states/verified_registry/verified_registry_actor_state.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v3::verified_registry {

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<AddVerifier>(),
      exportMethod<RemoveVerifier>(),
      exportMethod<AddVerifiedClient>(),
      exportMethod<UseBytes>(),
      exportMethod<RestoreBytes>(),
  };

}  // namespace fc::vm::actor::builtin::v3::verified_registry
