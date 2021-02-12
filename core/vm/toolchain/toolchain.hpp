/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor.hpp"
#include "vm/toolchain/address_matcher.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/actor/builtin/utils/init_actor_utils.hpp"

namespace fc::vm::toolchain {
  using actor::Actor;
  using actor::ActorVersion;
  using actor::CodeId;
  using version::NetworkVersion;
  using runtime::Runtime;
  using actor::builtin::utils::init::InitUtilsPtr;

  class Toolchain {
   public:
    static AddressMatcherPtr createAddressMatcher(ActorVersion version);
    static AddressMatcherPtr createAddressMatcher(
        const NetworkVersion &network_version);

    static InitUtilsPtr createInitActorUtils(Runtime &runtime);
  };
}  // namespace fc::vm::toolchain
