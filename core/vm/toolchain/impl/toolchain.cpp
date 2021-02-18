/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/toolchain.hpp"

#include "vm/toolchain/impl/address_matcher_v0.hpp"
#include "vm/toolchain/impl/address_matcher_v2.hpp"
#include "vm/toolchain/impl/address_matcher_v3.hpp"

#include "vm/actor/builtin/v0/init/init_actor_utils.hpp"
#include "vm/actor/builtin/v2/init/init_actor_utils.hpp"
#include "vm/actor/builtin/v3/init/init_actor_utils.hpp"

#include "vm/actor/builtin/v0/multisig/multisig_actor_utils.hpp"
#include "vm/actor/builtin/v2/multisig/multisig_actor_utils.hpp"
#include "vm/actor/builtin/v3/multisig/multisig_actor_utils.hpp"

#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor_utils.hpp"
#include "vm/actor/builtin/v2/verified_registry/verified_registry_actor_utils.hpp"
#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor_utils.hpp"

namespace fc::vm::toolchain {
  using namespace fc::vm::actor::builtin;

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

  AddressMatcherPtr Toolchain::createAddressMatcher(
      const NetworkVersion &network_version) {
    const auto version = actor::getActorVersionForNetwork(network_version);
    return createAddressMatcher(version);
  }

  InitUtilsPtr Toolchain::createInitActorUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::init::InitUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::init::InitUtils>(runtime);
      case ActorVersion::kVersion3:
        return std::make_shared<v3::init::InitUtils>(runtime);
    }
  }

  MultisigUtilsPtr Toolchain::createMultisigActorUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::multisig::MultisigUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::multisig::MultisigUtils>(runtime);
      case ActorVersion::kVersion3:
        return std::make_shared<v3::multisig::MultisigUtils>(runtime);
    }
  }

  VerifRegUtilsPtr Toolchain::createVerifRegUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::verified_registry::VerifRegUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::verified_registry::VerifRegUtils>(runtime);
      case ActorVersion::kVersion3:
        return std::make_shared<v3::verified_registry::VerifRegUtils>(runtime);
    }
  }
}  // namespace fc::vm::toolchain
