/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/utils/init_actor_utils.hpp"
#include "vm/actor/builtin/utils/multisig_actor_utils.hpp"
#include "vm/actor/builtin/utils/verified_registry_actor_utils.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/toolchain/address_matcher.hpp"

namespace fc::vm::toolchain {
  using actor::Actor;
  using actor::ActorVersion;
  using actor::CodeId;
  using actor::builtin::utils::init::InitUtilsPtr;
  using actor::builtin::utils::multisig::MultisigUtilsPtr;
  using actor::builtin::utils::verified_registry::VerifRegUtilsPtr;
  using runtime::Runtime;
  using version::NetworkVersion;

  class Toolchain {
   public:
    static AddressMatcherPtr createAddressMatcher(ActorVersion version);
    static AddressMatcherPtr createAddressMatcher(
        const NetworkVersion &network_version);

    static InitUtilsPtr createInitActorUtils(Runtime &runtime);
    static MultisigUtilsPtr createMultisigActorUtils(Runtime &runtime);
    static VerifRegUtilsPtr createVerifRegUtils(Runtime &runtime);
  };
}  // namespace fc::vm::toolchain
