/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/toolchain.hpp"

#include "vm/toolchain/impl/address_matcher_v0.hpp"
#include "vm/toolchain/impl/address_matcher_v2.hpp"
#include "vm/toolchain/impl/address_matcher_v3.hpp"

namespace fc::vm::toolchain {

  AddressMatcherPtr Toolchain::createAddressMatcher(ActorVersion version) {
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<AddressMatcherV0>();
      case ActorVersion::kVersion2:
        return std::make_shared<AddressMatcherV2>();
      case ActorVersion::kVersion3:
        return std::make_shared<AddressMatcherV3>();
    }
  }

  AddressMatcherPtr Toolchain::createAddressMatcher(const CodeId &actorCid) {
    const auto version = actor::getActorVersionForCid(actorCid);
    return createAddressMatcher(version);
  }

  AddressMatcherPtr Toolchain::createAddressMatcher(const Actor &actor) {
    return createAddressMatcher(actor.code);
  }

  AddressMatcherPtr Toolchain::createAddressMatcher(
      const NetworkVersion &network_version) {
    const auto version = actor::getActorVersionForNetwork(network_version);
    return createAddressMatcher(version);
  }
}  // namespace fc::vm::toolchain
