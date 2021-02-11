/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor.hpp"

#include "vm/toolchain/address_matcher.hpp"

namespace fc::vm::toolchain {
  using actor::Actor;
  using actor::ActorVersion;
  using actor::CodeId;
  using version::NetworkVersion;

  class Toolchain {
   public:
    static AddressMatcherPtr createAddressMatcher(ActorVersion version);
    static AddressMatcherPtr createAddressMatcher(const CodeId &actorCid);
    static AddressMatcherPtr createAddressMatcher(const Actor &actor);
    static AddressMatcherPtr createAddressMatcher(
        const NetworkVersion &network_version);
  };
}  // namespace fc::vm::toolchain
